assert(package.loadlib(os.getenv("EDBDIR").."/libexecute.so",
                       "lua_cryptdb_init"))()
local proto = assert(require("mysql.proto"))

--
-- Interception points provided by mysqlproxy
--


function read_auth()
    -- Use this instead of connect_server(), to get server name
    dprint("Connected " .. proxy.connection.client.src.name)
    CryptDB.connect(proxy.connection.client.src.name,
                    proxy.connection.server.dst.address,
                    proxy.connection.server.dst.port,
                    os.getenv("CRYPTDB_USER") or "root",
                    os.getenv("CRYPTDB_PASS") or "letmein",
                    os.getenv("CRYPTDB_SHADOW") or "/var/lib/shadow-mysql")
    -- EDBClient uses its own connection to the SQL server to set up UDFs
    -- and to manipulate multi-principal state.  (And, in the future, to
    -- store its schema state for single- and multi-principal operation.)
end

function disconnect_client()
    dprint("Disconnected " .. proxy.connection.client.src.name)
    CryptDB.disconnect(proxy.connection.client.src.name)
end

function read_query(packet)
    local status, err = pcall(read_query_real, packet)
    if status then
        return err
    else
        print("read_query: " .. err)
        return proxy.PROXY_SEND_QUERY
    end
end

function read_query_result(inj)
    local status, err = pcall(read_query_result_real, inj)
    if status then
        return err
    else
        print("read_query_result: " .. err)
        return proxy.PROXY_SEND_RESULT
    end
end

--
-- Helper functions
--

RES_IGNORE   = 1
RES_DECRYPT  = 2
RES_ROLLBACK = 3

function dprint(x)
    if os.getenv("CRYPTDB_PROXY_DEBUG") then
        print(x)
    end
end

function read_query_real(packet)
    local query = string.sub(packet, 2)
    print("read_query: " .. query)

    if string.byte(packet) == proxy.COM_QUERY then
        do_rollback, new_queries =
            CryptDB.rewrite(proxy.connection.client.src.name, query)

        if not new_queries then
            proxy.response.type = proxy.MYSQLD_PACKET_ERR
            proxy.response.errmsg = "query failed"
            return proxy.PROXY_SEND_RESULT
        end

        if table.maxn(new_queries) == 0 then
            proxy.response.type = proxy.MYSQLD_PACKET_OK
            return proxy.PROXY_SEND_RESULT
        end

        dprint(" ")
        for i, v in pairs(new_queries) do
            dprint("rewritten query[" .. i .. "]: " .. v)
            local result_key
            if i == table.maxn(new_queries) then
                if true == do_rollback then
                    result_key = RES_ROLLBACK
                else
                    result_key = RES_DECRYPT
                end
            else
                result_key = RES_IGNORE
            end
            proxy.queries:append(result_key,
                                 string.char(proxy.COM_QUERY) .. v,
                                 { resultset_is_needed = true })
        end

        return proxy.PROXY_SEND_QUERY
    elseif string.byte(packet) == proxy.COM_QUIT then
        -- do nothing
    else
        print("unexpected packet type " .. string.byte(packet))
    end
end

function read_query_result_real(inj)
    local client = proxy.connection.client.src.name

    if inj.id == RES_IGNORE then
        return proxy.PROXY_IGNORE_RESULT
    elseif inj.id == RES_ROLLBACK then
        CryptDB.rollbackOnionAdjust(client)

        proxy.response.type = proxy.MYSQLD_PACKET_ERR
        proxy.response.errmsg = "Proxy did ROLLBACK"
        -- ER_LOCK_DEADLOCK::error = 1213
        proxy.response.errcode = 1213
        -- ER_LOCK_DEADLOCK::sqlstate = 40001
        proxy.response.sqlstate = 40001
        return proxy.PROXY_SEND_RESULT
    elseif inj.id == RES_DECRYPT then
        local resultset = inj.resultset

        if resultset.query_status == proxy.MYSQLD_PACKET_ERR then
            CryptDB.queryFailure(client)

            local err = proto.from_err_packet(resultset.raw)
            proxy.response.type = proxy.MYSQLD_PACKET_ERR
            proxy.response.errmsg = err.errmsg
            proxy.response.errcode = err.errcode
            proxy.response.sqlstate = err.sqlstate
        else
            local fields = {}
            local rows = {}
            local query = inj.query:sub(2)

            -- Handle the backend of the query.
            decryptp, res_ptr =
                CryptDB.epilogue(client)
            if res_ptr then
                dfields, drows = CryptDB.passDecryptedPtr(client, res_ptr)
            else
                 -- for DEMO: printing results
                local f_names = ""
                local r = ""

                -- mysqlproxy doesn't return real lua arrays, so re-package
                local resfields = resultset.fields
                for i = 1, #resfields do
                    rfi = resfields[i]
                    fields[i] = { type = rfi.type, name = rfi.name }
                    f_names = f_names .. "|" .. rfi.name
                end

                local resrows = resultset.rows
                if resrows then
                    for row in resrows do
                        table.insert(rows, row)
                    end
                end

_=[[
                -- DEMO
                if #rows > 0 then
                   dprint(" ")
                   dprint("Results from server:")
                end
                dprint(f_names)
                for i = 1, #rows do
                    for j = 1, #rows[i] do
                        r = r .. "|" .. rows[i][j]
                    end
                    dprint(r)
                    r = ""
                end
]]
                if true == decryptp then
                    dfields, drows =
                        CryptDB.decrypt(client, fields, rows)
                else
                    -- FIXME: This data may be illformed as we don't do
                    -- the fixups with respect to NULL that take place in
                    -- CryptDB.decrypt.
                    dfields = fields
                    drows = rows
                end
            end

            if dfields and drows then
_=[[
                -- DEMO
                f_names = ""
                r = ""
                for i = 1, #dfields do
                    f_names = f_names .. " | " .. dfields[i].name
                end

                if #drows > 0 then
                   dprint(" ")
                   dprint("Decrypted results:")
                end
                dprint(f_names)

                for i = 1, #drows do
                    for j = 1, #drows[i] do
                        r = r .. " | " .. drows[i][j]
                    end
                    dprint(r)
                    r = "" 
                end
]]
                proxy.response.type = proxy.MYSQLD_PACKET_OK
                proxy.response.affected_rows = resultset.affected_rows
                proxy.response.insert_id = resultset.insert_id
                if table.maxn(dfields) > 0 then
                    proxy.response.resultset = { fields = dfields, rows = drows }
                end
            else
                proxy.response.type = proxy.MYSQLD_PACKET_ERR
                proxy.response.errmsg = "could not decrypt result"
            end
        end

        return proxy.PROXY_SEND_RESULT
    else
        print("unexpected inj.id " .. inj.id)
    end
end
