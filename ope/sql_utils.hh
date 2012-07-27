#pragma once

#include <util/util.hh>
#include <vector>
#include <string>

#include <sql_select.h>
#include <sql_delete.h>
#include <sql_insert.h>
#include <sql_update.h>


class ResType {
 public:
    explicit ResType(bool okflag = true) : ok(okflag) {}

    bool ok;  // query executed successfully
    std::vector<std::string> names;
    std::vector<enum_field_types> types;
    std::vector<std::vector<Item*> > rows;
    AutoInc ai;
};

bool isTableField(std::string token);
std::string fullName(std::string field, std::string name);

char * make_thd_string(const std::string &s, size_t *lenp = 0);

std::string ItemToString(Item * i);
