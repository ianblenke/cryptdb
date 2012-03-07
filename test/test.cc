#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <iostream>
#include <istream>
#include <fstream>
#include <iomanip>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/wait.h>

#include <crypto-old/OPE.hh>
#include <crypto-old/HGD.hh>
#include <crypto-old/CryptoManager.hh>

#include <edb/EDBProxy.hh>
#include <edb/AccessManager.hh>
#include <edb/Connect.hh>

#include <util/util.hh>
#include <util/params.hh>
#include <util/cryptdb_log.hh>

#include <test/test_utils.hh>
#include <test/TestSinglePrinc.hh>
#include <test/TestMultiPrinc.hh>
#include <test/TestCrypto.hh>
#include <test/TestAccessManager.hh>
#include <test/TestProxy.hh>
#include <test/TestQueries.hh>
#include <test/TestNotConsider.hh>


using namespace std;
using namespace NTL;

static string __attribute__((unused))
padPasswd(const string &s)
{
    string r = s;
    r.resize(AES_KEY_BYTES, '0');
    return r;
}


static clock_t timeStart;

static void
startTimer ()
{
    timeStart = time(NULL);
}

static double
readTimer()
{
    clock_t currentTime = time(NULL);
    double res = (double) (currentTime - timeStart) * 1000.0;
    return res;
}

static void __attribute__((unused))
test_OPE()
{

    cerr << "\n OPE test started \n";

    const unsigned int OPEPlaintextSize = 32;
    const unsigned int OPECiphertextSize = 128;

    unsigned char key[AES_KEY_SIZE/
                      bitsPerByte] =
                      {158, 242, 169, 240, 255, 166, 39, 177, 149, 166, 190, 237, 178, 254, 187,
                              40};

    cerr <<"Key is " << stringToByteInts(string((char *) key,
            AES_KEY_SIZE/bitsPerByte)) << "\n";

    OPE * ope = new OPE((const char *) key, OPEPlaintextSize,
            OPECiphertextSize);

    unsigned char plaintext[OPEPlaintextSize/bitsPerByte] = {74, 95, 221, 84};
    string plaintext_s = string((char *) plaintext,
            OPEPlaintextSize/bitsPerByte);

    string ciphertext = ope->encrypt(plaintext_s);
    string decryption = ope->decrypt(ciphertext);

    LOG(test) << "Plaintext is " << stringToByteInts(plaintext_s);
    LOG(test) << "Ciphertext is " << stringToByteInts(ciphertext);
    LOG(test) << "Decryption is " << stringToByteInts(decryption);

    myassert(plaintext_s == decryption, "OPE test failed \n");

    cerr << "OPE Test Succeeded \n";

    unsigned int tests = 100;
    cerr << "Started " << tests << " tests \n ";

    clock_t encTime = 0;
    clock_t decTime = 0;
    clock_t currTime;
    time_t startTime = time(NULL);
    for (unsigned int i = 0; i < tests; i++) {

        string ptext = randomBytes(OPEPlaintextSize/bitsPerByte);
        currTime = clock();
        string ctext =  ope->encrypt(ptext);
        encTime += clock() - currTime;
        currTime = clock();
        ope->decrypt(ctext);
        decTime += clock() - currTime;
    }
    time_t endTime = time(NULL);
    cout << "(time): encrypt/decrypt take  " <<
            (1.0 * (double) (endTime-startTime))/(2.0*tests) << "s \n";
    cout << "encrypt takes on average " <<
            ((double) encTime*1000.0)/(1.0*CLOCKS_PER_SEC*tests) << "ms \n";
    cout << "decrypt takes on average " <<
            ((double) decTime*1000.0)/(1.0*CLOCKS_PER_SEC*tests) << "ms \n";

}

static void __attribute__((unused))
evaluate_AES(const TestConfig &tc, int argc, char ** argv)
{

    if (argc!=2) {
        cout << "usage ./test noTests \n";
        exit(1);
    }

    unsigned int notests = 10;

    string key = randomBytes(AES_KEY_SIZE/bitsPerByte);
    string ptext = randomBytes(AES_BLOCK_SIZE/bitsPerByte);

    AES_KEY aesKey;
    AES_set_encrypt_key((const uint8_t *) key.c_str(), AES_KEY_SIZE, &aesKey);

    timeval startTime, endTime;

    unsigned int tests = 1024*1024;

    for (unsigned int j = 0; j < notests; j++) {
        gettimeofday(&startTime, NULL);

        for (unsigned int i = 0; i < tests; i++) {
            unsigned char ctext[AES_BLOCK_SIZE/bitsPerByte];
            AES_encrypt((const uint8_t *) ptext.c_str(), ctext, &aesKey);
            ptext = string((char *) ctext, AES_BLOCK_BYTES);
        }

        gettimeofday(&endTime, NULL);

        cerr << (tests*16.0)/(1024*1024) << "  "
                << timeInSec(startTime, endTime) << "\n";
        tests = (uint) ((double) tests * 1.2);
    }
    cout << "result " << ptext  << "\n";

}

static void __attribute__((unused))
test_HGD()
{
    unsigned int len = 16;     //bytes
    unsigned int bitsPrecision = len * bitsPerByte + 10;
    ZZ K = ZZFromString(randomBytes(len));
    ZZ N1 = ZZFromString(randomBytes(len));
    ZZ N2 = ZZFromString(randomBytes(len));
    ZZ SEED = ZZFromString(randomBytes(len));

    ZZ sample = HGD(K, N1, N2, SEED, len*bitsPerByte, bitsPrecision);

    LOG(test) << "N1 is " << stringToByteInts(StringFromZZ(N1));
    LOG(test) << "N2 is " << stringToByteInts(StringFromZZ(N2));
    LOG(test) << "K is " << stringToByteInts(StringFromZZ(K));
    LOG(test) << "HGD sample is " << stringToByteInts(StringFromZZ(sample));

    unsigned int tests = 1000;
    cerr << " Started " << tests << " tests \n";

    clock_t totalTime = 0;     //in clock ticks

    for (unsigned int i = 0; i< tests; i++) {
        K = N1+ N2+1;
        while (K > N1+N2) {
            cerr << "test " << i << "\n";
            K = ZZFromString(randomBytes(len));
            N1 = ZZFromString(randomBytes(len));
            N2 = ZZFromString(randomBytes(len));
            SEED = ZZFromString(randomBytes(len));
        }

        clock_t currentTime = clock();
        sample = HGD(K, N1, N2, SEED, len*bitsPerByte, bitsPrecision);

        totalTime += clock() - currentTime;
    }

    cerr << "average milliseconds per test is " <<
            ((double) totalTime * 1000.0) / ((double) tests * CLOCKS_PER_SEC) << "\n";
}
/*
   void test_EDBProxy_noSecurity() {
    EDBProxy  e =  EDBProxy((char *)"dbname = postgres", false);

    PGresult * res = e.execute("SELECT * FROM pg_database ;");

    int nFields = PQnfields(res);
    for (int i = 0; i < nFields; i++)
        printf("%-15s", PQfname(res, i));
    printf("\n\n");
 */
/* next, print out the rows */
/*    for (int i = 0; i < PQntuples(res); i++)
    {
        for (int j = 0; j < nFields; j++)
            printf("%-15s", PQgetvalue(res, i, j));
        printf("\n");
    }

    PQclear(res);



   }*/
/*
static void __attribute__((unused))
evaluateMetrics(const TestConfig &tc, int argc, char ** argv)
{

    if (argc != 4) {
        printf("usage: ./test noRecords tests haveindex?(0/1) ");
        exit(1);
    }

    unsigned int noRecords = atoi(argv[1]);
    unsigned int tests = atoi(argv[2]);

    time_t timerStart, timerEnd;

    EDBProxy * cl = new EDBProxy(tc.host, tc.user, tc.pass, tc.db);

    cl->execute(
            "CREATE TABLE testplain (field1 int, field2 int, field3 int);");
    cl->execute(
            "CREATE TABLE testcipher (field1 varchar(16), field2 varchar(16), field3 varchar(16));");

    timerStart = time(NULL);
    //populate both tables with increasing values
    for (unsigned int i = 0; i < noRecords; i++) {
        string commandPlain = "INSERT INTO testplain VALUES (";
        string value = StringFromVal(i);
        commandPlain = commandPlain + value + ", " + value + "," + value +
                ");";

        cl->execute(commandPlain);

    }
    timerEnd = time(NULL);
    printf("insert plain average time %f ms \n",
            (1000.0 * (double) (timerEnd-timerStart))/(noRecords*1.0));

    timerStart = time(NULL);
    for (unsigned int i = 0; i < noRecords; i++) {
        string commandCipher = "INSERT INTO testcipher VALUES (";
        string valueBytes ="'" +
                StringFromVal(i, AES_BLOCK_BITS/
                        bitsPerByte) + "'";

        commandCipher = commandCipher + valueBytes + ", " + valueBytes +
                ", " + valueBytes+ ");";
        cl->execute(commandCipher);

    }
    timerEnd = time(NULL);
    printf("insert cipher average time %f ms \n",
            (1000.0 * (double) (timerEnd-timerStart))/(noRecords*1.0));

    if (atoi(argv[3]) == 1) {
        cout << "create index";
        cl->execute("CREATE INDEX indplain ON testplain (field1) ;");
        cl->execute("CREATE INDEX indcipher ON testcipher (field1) ;");
    }

    timerStart = time(NULL);
    //equality selection
    for (unsigned int i = 0; i < tests; i++) {
        int j = rand() % noRecords;
        string commandPlain = "SELECT * FROM testplain WHERE field1 = ";
        string value = StringFromVal(j);
        commandPlain += value  + ";";
        //cout << "CL " << clock() << "\n";
        cl->execute(commandPlain);

    }
    timerEnd = time(NULL);

    printf("select plain time %f ms \n",
            (1000.0 * (double) (timerEnd-timerStart))/(tests*1.0));

    timerStart = time(NULL);
    //equality selection
    for (unsigned int i = 0; i < tests; i++) {
        int j = rand() % noRecords;
        string commandCipher = "SELECT * FROM testcipher WHERE field1 = ";
        string valueBytes = "'" +
                StringFromVal(j, AES_BLOCK_BITS/
                        bitsPerByte) + "'";

        commandCipher = commandCipher + valueBytes + ";";
        cl->execute(commandCipher);

    }
    timerEnd = time(NULL);

    printf("cipher average time %f ms \n",
            (1000.0* (double) (timerEnd-timerStart))/(tests*1.0));

    / *
       timerStart = time(NULL);
       //inequality selection
       for (int i = 0; i < tests; i++) {
       int leftJ = rand() % noRecords;
       //int rightJ = rand() % noRecords;

       //if (leftJ > rightJ) {
       //    int aux = leftJ;
       //    leftJ = rightJ;
       //    rightJ = aux;
       //}
       int rightJ = leftJ + (rand() % 50);

       string commandPlain = "SELECT * FROM testplain WHERE field1 > ";
       string leftJBytes = StringFromVal(leftJ);
       string rightJBytes = StringFromVal(rightJ);

       commandPlain = commandPlain + leftJBytes + " AND field1 < " +
          rightJBytes + ";";

       cl->execute(commandPlain);
       }
       timerEnd = time(NULL);
       printf("range select plain %f ms \n",
          (1000.0*(timerEnd-timerStart))/(tests*1.0));
* /
    / *
       timerStart = time(NULL);
       //inequality selection
       for (int i = 0; i < tests; i++) {
       int leftJ = rand() % noRecords;
       //int rightJ = rand() % noRecords;

       //if (leftJ > rightJ) {
       //    int aux = leftJ;
       //    leftJ = rightJ;
       //    rightJ = aux;
       //}
       int rightJ = leftJ + (rand() % 50);

       string commandCipher = "SELECT * FROM testcipher WHERE field1 > ";
       string leftJBytes = "'" + StringFromVal(leftJ,
          AES_BLOCK_BITS/bitsPerByte) + "'";
       string rightJBytes = "'" + StringFromVal(rightJ,
          AES_BLOCK_BITS/bitsPerByte) + "'";

       commandCipher = commandCipher + leftJBytes + " AND field1 < " +
          rightJBytes + ";";

       cl->execute(commandCipher);
       }
       timerEnd = time(NULL);
       printf("range select cipher %f ms \n",
          (1000.0*(timerEnd-timerStart))/(tests*1.0));
     * /

    cl->execute("DROP TABLE testplain;");
    cl->execute("DROP TABLE testcipher;");

}
*/
//tests protected methods of EDBProxy
class tester : public EDBProxy {
public:
    tester(const TestConfig &tc, const string &masterKey) : EDBProxy(tc.host,
            tc.user,
            tc.pass,
            tc.db,
            0, false)
    {
        setMasterKey(masterKey);
    }
    tester(const TestConfig &tc) : EDBProxy(tc.host, tc.user, tc.pass, tc.db)
    {
    };
    void testClientParser();
    void loadData(EDBProxy * cl, string workload, int logFreq);

    //void testMarshallBinary();
};

void
tester::testClientParser()
{
    cerr << "testClientParser uses old EDBProxy -- not run" << endl;

    /*list<string> queries = list<string>();
    //queries.push_back(string("CREATE TABLE people (id integer, age integer,
            // name integer);"));
    queries.push_back(string(
            "CREATE TABLE city (name integer, citizen integer);"));
    queries.push_back(string(
            "CREATE TABLE emp (id integer, name text, age integer, job text);"));
    //queries.push_back(string("SELECT city.citizen FROM people, city WHERE
    // city.citizen = people.name ; "));
    //queries.push_back(string("INSERT INTO people VALUES (5, 23, 34);"));
    //queries.push_back(string("INSERT INTO city VALUES (34, 24);"));
    //queries.push_back(string("SELECT people.id FROM people WHERE people.id =
    // 5 AND people.id = people.age ;"));
    //queries.push_back(string("DROP TABLE people;"));

    list<int> expectedCount = list<int>();
    expectedCount.push_back(1);
    expectedCount.push_back(1);
    expectedCount.push_back(4);
    expectedCount.push_back(2);
    expectedCount.push_back(4);
    expectedCount.push_back(1);

    list<string> expected = list<string>();

    expected.push_back("CREATE TABLE table0 (  field0DET integer, field0OPE bigint, field1DET integer, field1OPE bigint, field2DET integer, field2OPE bigint );");
    expected.push_back("CREATE TABLE table1 (  field0DET integer, field0OPE bigint, field1DET integer, field1OPE bigint );");
    expected.push_back("UPDATE table1 SET field1DET = DECRYPT(0);");
    expected.push_back("UPDATE table0 SET field2DET = DECRYPT(0);");
    expected.push_back("UPDATE table1 SET field1DET = EQUALIZE(0);");
    expected.push_back("SELECT  table1.field1DET FROM  table0, table1 WHERE  table1.field1DET  =  table0.field2DET ;");
    expected.push_back("UPDATE table1 SET field1DET = 5;");
    expected.push_back("UPDATE table1 SET field1OPE = 5;");
    expected.push_back("UPDATE table0 SET field0DET = DECRYPT(0);");
    expected.push_back("UPDATE table0 SET field1DET = DECRYPT(0);");
    expected.push_back("UPDATE table0 SET field0DET = EQUALIZE(0);");
    expected.push_back("SELECT  table0.field0DET FROM  table0 WHERE  table0.field0DET  = 5 AND  table0.field0DET  =  table0.field1DET ;");
    expected.push_back("DROP TABLE table0;");

    list<string>::iterator it = queries.begin();

    for (; it != queries.end(); it++) {   //TODO: check against expected...at
        // this point is more of a manual
        // check
        bool temp;
        list<string> response = rewriteEncryptQuery(it->c_str(), temp);
        LOG(test) << "query issued/response: " << *it << ", " << toString(response, stringToByteInts);
    }

    exit();
    cerr << "TEST TRANSLATOR PASSED \n";
    */
}

static void __attribute__((unused))
testCryptoManager()
{

    string masterKey = randomBytes(AES_KEY_BYTES);
    CryptoManager * cm = new CryptoManager(masterKey);

    cerr << "TEST CRYPTO MANAGER \n";

    //test marshall and unmarshall key
    string m = cm->marshallKey(masterKey);
    LOG(test) << "master key is " << stringToByteInts(masterKey);
    LOG(test) << "marshall is " << m;
    string masterKey2 = cm->unmarshallKey(m);

    myassert(masterKey == masterKey2, "marshall test failed");

    LOG(test) << "key for field1: "
            << stringToByteInts(cm->getKey("field1", SECLEVEL::SEMANTIC_OPE));
    LOG(test) << "key for table5.field12OPE:"
            << stringToByteInts(cm->getKey("table5.field12OPE", SECLEVEL::SEMANTIC_OPE));

    //test SEM
    AES_KEY * aesKey = cm->get_key_SEM(masterKey);
    uint64_t salt = 3953954;
    uint32_t value = 5;
    uint32_t eValue = cm->encrypt_SEM(value, aesKey, salt);
    cerr << "\n sem encr of " << value << " is " << eValue << "with salt " <<
            salt << " and decr of encr is " <<
            cm->decrypt_SEM(eValue, aesKey, salt) <<"\n";
    myassert(cm->decrypt_SEM(eValue, aesKey,
            salt) == value,
            "decrypt of encrypt does not return value");

    cerr << "SEMANTIC " << (int) SECLEVEL::SEMANTIC_OPE << "\n";

    uint64_t value2 = 10;
    uint64_t eValue2 = cm->encrypt_SEM(value2, aesKey, salt);
    cerr << "sem encr of " << value2 << " is " << eValue2 <<
            " and signed is " << (int64_t) eValue2 << "\n";
    myassert(cm->decrypt_SEM(eValue2, aesKey,
            salt) == value2,
            "decrypt of encrypt does not return correct value for uint64_t");

    cerr << "0, " << (int64_t) eValue2 << ", " << m << ", " << salt << "\n";

    OPE * ope = cm->get_key_OPE(masterKey);
    uint64_t eOPE = cm->encrypt_OPE(value, ope);
    cerr << "encryption is eOPE " << eOPE << " \n";
    myassert(cm->decrypt_OPE(eOPE, ope) == value, "ope failed");

    cerr << "TEST CRYPTO MANAGER PASSED \n";

}

static void
testTrain(const TestConfig &tc, int ac, char **a) {
    /*EDBProxy * pr = new EDBProxy(tc.host, tc.user, tc.pass, tc.db, tc.port, false, true);

    pr->plain_execute("drop database cryptdbtest;");
    pr->plain_execute("create database cryptdbtest;");
    pr->plain_execute("use cryptdbtest;");

    string mkey = randomBytes(AES_BLOCK_BYTES);
    pr->setMasterKey(mkey);

    pr->execute("train 1 testtraincreate testpatterns 1");


    LOG(test) << "\n \n ";
    LOG(test) << "done training";
    LOG(test) << "\n \n ";

    ifstream fc("testtraincreate");

    while (!fc.eof()) {
        string query = getQuery(fc);
        pr->plain_execute(query);
    }

    fc.close();

    ifstream f("testpatterns");

    while (!f.eof()) {
        string query = getQuery(f);

        ResType r1 = pr->execute(query);
        ResType r2 = pr->plain_execute(query);

        assert_s(r1.ok, "query failed: " + query);
        assert_s(r2.ok, "query failed for plain: " + query);

        assert_s(match(r1, r2), "plain and trained cryptdb give different results on query " + query);
        LOG(test) << "MATCH!";
    }

    f.close();
    cerr << "Test train passed\n";
    */
}
//do not change: has been used in creating the DUMPS for experiments
const uint64_t mkey = 113341234;
/*
static void __attribute__((unused))
evalImproveSummations(const TestConfig &tc)
{
    string masterKey = BytesFromInt(mkey, AES_KEY_BYTES);
    string host = tc.host;
    string user = tc.user;
    string db = tc.db;
    string pwd = tc.pass;
    cerr << "connecting to host " << host << " user " << user << " pwd " <<
            pwd << " db " << db << endl;
    EDBProxy * cl = new EDBProxy(host, user, pwd, db);
    cl->setMasterKey(masterKey);
    cl->VERBOSE = true;

    cl->execute("CREATE TABLE test_table (id enc integer,  name enc text)");
    unsigned int no_inserts = 100;
    unsigned int no_sums = 20;

    for (unsigned int i = 0; i < no_inserts; i++) {
        cl->execute(string("INSERT INTO test_table VALUES (") +
                StringFromVal(i) + " , 'ana');");
    }

    startTimer();
    for (unsigned int i = 0; i < no_sums; i++) {
        cl->execute("SELECT sum(id) FROM test_table;");
    }
    double time = readTimer();

    cerr << "time per sum: " << time/(1.0*no_sums) << " ms \n";

}
*/
static void
interactiveTest(const TestConfig &tc, int ac, char **av)
{
    cerr << "interactiveTest uses old EDBProxy; please use obj/parser/cdb_test" << endl;
    /*
    cout << "\n ---------   CryptDB ---------- \n \n";

    cout << "To exit, hit \\q\n";

    string masterKey = BytesFromInt(mkey, AES_KEY_BYTES);
    string host = tc.host;
    string user = tc.user;
    string db = tc.db;
    string pwd = tc.pass;
    cerr << "connecting to host " << host << " user " << user << " pwd " <<
            pwd << " db " << db << endl;
    EDBProxy * cl;

    if (ac >= 2 && string(av[1]) == "multi") {
        cl = new EDBProxy(host, user, pwd, db, 0, true);
    } else {
        cl = new EDBProxy(host, user, pwd, db,0, false);
    }
    cl->setMasterKey(masterKey);
    cl->VERBOSE = true;

    //cl->execute("CREATE TABLE phpbb_forums (forum_id mediumint(8) UNSIGNED NOT NULL auto_increment, parent_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL, left_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,  right_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL, forum_parents mediumtext NOT NULL, forum_name encfor forum_id varchar(255) NOT NULL, forum_desc text NOT NULL, forum_desc_bitfield varchar(255) DEFAULT '' NOT NULL, forum_desc_options int(11) UNSIGNED DEFAULT '7' NOT NULL, forum_desc_uid varchar(8) DEFAULT '' NOT NULL, forum_link varchar(255) DEFAULT '' NOT NULL, forum_password varchar(40) DEFAULT '' NOT NULL, forum_style mediumint(8) UNSIGNED DEFAULT '0' NOT NULL, forum_image encfor forum_id varchar(255) NOT NULL, forum_rules text NOT NULL, forum_rules_link varchar(255) DEFAULT '' NOT NULL, forum_rules_bitfield varchar(255) DEFAULT '' NOT NULL, forum_rules_options int(11) UNSIGNED DEFAULT '7' NOT NULL, forum_rules_uid varchar(8) DEFAULT '' NOT NULL, forum_topics_per_page tinyint(4) DEFAULT '0' NOT NULL, forum_type tinyint(4) DEFAULT '0' NOT NULL, forum_status tinyint(4) DEFAULT '0' NOT NULL, forum_posts mediumint(8) UNSIGNED DEFAULT '0' NOT NULL, forum_topics mediumint(8) UNSIGNED DEFAULT '0' NOT NULL, forum_topics_real mediumint(8) UNSIGNED DEFAULT '0' NOT NULL, forum_last_post_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL, forum_last_poster_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL, forum_last_post_subject encfor forum_id det varchar(255) NOT NULL, forum_last_post_time int(11) UNSIGNED DEFAULT '0' NOT NULL, forum_last_poster_name varchar(255) DEFAULT '' NOT NULL, forum_last_poster_colour varchar(6) DEFAULT '' NOT NULL, forum_flags tinyint(4) DEFAULT '32' NOT NULL, forum_options int(20) UNSIGNED DEFAULT '0' NOT NULL, display_subforum_list tinyint(1) UNSIGNED DEFAULT '1' NOT NULL, display_on_index tinyint(1) UNSIGNED DEFAULT '1' NOT NULL, enable_indexing tinyint(1) UNSIGNED DEFAULT '1' NOT NULL, enable_icons tinyint(1) UNSIGNED DEFAULT '1' NOT NULL, enable_prune tinyint(1) UNSIGNED DEFAULT '0' NOT NULL, prune_next int(11) UNSIGNED DEFAULT '0' NOT NULL, prune_days mediumint(8) UNSIGNED DEFAULT '0' NOT NULL, prune_viewed mediumint(8) UNSIGNED DEFAULT '0' NOT NULL, prune_freq mediumint(8) UNSIGNED DEFAULT '0' NOT NULL, PRIMARY KEY (forum_id), KEY left_right_id (left_id, right_id), KEY forum_lastpost_id (forum_last_post_id));");
    //cl->execute("INSERT INTO phpbb_forums (forum_name, forum_desc, left_id, right_id, parent_id, forum_type, forum_posts, forum_topics, forum_topics_real, forum_last_post_id, forum_last_poster_id, forum_last_poster_name, forum_last_poster_colour, forum_last_post_time, forum_link, forum_password, forum_image, forum_rules, forum_rules_link, forum_rules_uid, forum_desc_uid, prune_days, prune_viewed, forum_parents) VALUES ('Your first category', '', 1, 4, 0, 0, 1, 1, 1, 1, 2, 'Admin', 'AA0000', 972086460, '', '', '', '', '', '', '', 0, 0, '');");


    streamsize len = 500;
    char *cmd = new char[len];

    for (;; ) {

        cout << "CryptDB=# ";
        cin.getline(cmd, len);
        if (cin.eof())
            break;

        string commandS = string(cmd);
        if (commandS == "") {
            break;
        }

        if (commandS.compare("\\q") == 0) {
            break;
        } else if (commandS.compare("load cryptapp") == 0) {
            cl->execute(
                    "CREATE TABLE users (id integer accessto uname, uname text givespsswd); ");
            cl->execute(
                    "CREATE TABLE info (id integer equals users.id , creditcard integer encfor id); ");
            cl->execute(
                    "INSERT INTO activeusers VALUES ('alice', 'secretA');");
            cl->execute("INSERT INTO activeusers VALUES ('bob', 'secretB');");
            cl->execute(
                    "INSERT INTO activeusers VALUES ('chris', 'secretC');");
            cl->execute("INSERT INTO activeusers VALUES ('dan', 'secretD');");
            cl->execute("INSERT INTO users VALUES (1, 'alice');");
            cl->execute("INSERT INTO users VALUES (2, 'bob');");
            cl->execute("INSERT INTO users VALUES (3, 'chris');");
            cl->execute("INSERT INTO users VALUES (4, 'dan');");
            cl->execute("INSERT INTO info VALUES (1, 111);");
            cl->execute("INSERT INTO info VALUES (2, 222);");
            cl->execute("INSERT INTO info VALUES (3, 333);");
            cl->execute("INSERT INTO info VALUES (4, 444);");
        } else if (commandS.compare("load people;") == 0) {
            int noInserts = 15;
            for (int i = 0; i < noInserts; i++) {
                unsigned int val1 = rand() % 10;
                unsigned int val2 = rand() % 10;
                string qq = "INSERT INTO people VALUES ( " +
                        strFromVal(val1) + ", " + strFromVal(val2) +
                        ");";
                cl->execute(qq);
            }
        } else if (commandS.compare("load all emp;") == 0) {
            cl->execute("CREATE TABLE emp (id integer, jobid integer);");

            int noInserts = 20;
            for (int i = 0; i < noInserts; i++) {
                unsigned int val1 = rand() % 10;
                unsigned int val2 = rand() % 10;
                string qq = "INSERT INTO emp VALUES ( " + strFromVal(val1) +
                        ", " + strFromVal(val2) + ");";
                cl->execute(qq);
            }
        } else if (commandS.find("login") == 0) {
            list<string> words = parse(commandS, delimsStay, delimsGo,
                    keepIntact);
            list<string>::iterator wordsIt = words.begin();
            wordsIt++;
            string uname = getVal(wordsIt);
            string p = getVal(wordsIt);
            string query = "INSERT INTO activeusers VALUES ('" + uname +
                    "' , '" + p + "' );";
            cl->execute(query);
        } else if (commandS.find("logout") == 0) {
            list<string> words = parse(commandS, delimsStay, delimsGo,
                    keepIntact);
            list<string>::iterator wordsIt = words.begin();
            wordsIt++;
            string uname = getVal(wordsIt);
            string query = "DELETE FROM activeusers WHERE uname = '" +
                    uname + "';";
            cl->execute(query);
        } else if (commandS.compare("debug;") == 0) {
            //assert_res(cl->execute(), "failed");

            cl->plain_execute("DROP TABLE IF EXISTS table0, table1");
            cerr << "here \n";
            assert_res(cl->execute(
                    "CREATE TABLE t(id enc integer, name enc text);"),
                    "failed");

            assert_res(cl->execute(
                    "INSERT INTO t VALUES (1, '5');"), "failed");

            assert_res(cl->execute(  "INSERT INTO t VALUES (2, 'alice');"), "failed");

            assert_res(cl->execute( "UPDATE t SET id = 10;"),
                    "failed");

            assert_res(cl->execute( "SELECT * FROM t;"),
                                                    "failed");

            assert_res(cl->execute(  "CREATE TABLE t2 (age enc integer);"),
                                                    "failed");
            assert_res(cl->execute(  "INSERT INTO t2 VALUES (5);"), "failed");

                        assert_res(cl->execute(
                                        "SELECT t2.age FROM test_insert, t2 WHERE test_insert.age = t2.age;"), "failed");

                        //assert_res(cl->execute("SELECT * FROM test_insert;"), "failed");

            // assert_res(cl->execute(
            //              "SELECT sum(id) FROM hi;"), "failed");


             assert_res(cl->execute(
                         "INSERT INTO hi VALUES (1, 'lauren');"), "failed");
               assert_res(cl->execute(
                         "INSERT INTO hi VALUES (1, 'aaa');"), "failed");
               assert_res(cl->execute(
                         "INSERT INTO hi VALUES (1, 'xxhello');"), "failed");
               assert_res(cl->execute(
                         "INSERT INTO hi VALUES (1, 'ydan');"), "failed");
               assert_res(cl->execute(
                         "INSERT INTO hi VALUES (1, 'blauren');"), "failed");
               assert_res(cl->execute(
                         "INSERT INTO hi VALUES (1, 'naaa');"), "failed");
               assert_res(cl->execute(
                         "INSERT INTO hi VALUES (1, 'hello');"), "failed");
            assert_res(cl->execute(
                         "SELECT * FROM hi, hi2 WHERE hi.name = hi2.name ;"),
                     "failed");

            //debugging of DECRYPTFIRST mode

            //cl->plain_execute("DROP TABLE IF EXISTS hi;");
            //assert_res(cl->execute("CREATE TABLE hi (id enc integer, name
            // text);"), "failed");
            //    assert_res(cl->execute("INSERT INTO hi VALUES (3, '5');"),
            // "failed");
            //    assert_res(cl->execute("SELECT * FROM hi;"), "failed");
            //    assert_res(cl->execute("SELECT id, name AS n FROM hi WHERE id =
            // 3;"), "failed");
            //    assert_res(cl->execute("SELECT * FROM hi WHERE id > 2;"),
            // "failed");
            //assert_res(cl->execute("SELECT * FROM hi;"), "failed");

            //GENERAL MULTI-KEY DEBUGGING

            
               cl->plain_execute("DROP TABLE IF EXISTS t1, users,
                  pwdcryptdb__users, cryptdb_public, cryptdb_active0;");
               assert_res(cl->execute("CREATE TABLE t1 (id integer, post encfor
                  id det text, age encfor id ope bigint);"), "failed");
               assert_res(cl->execute("CREATE TABLE users (id equals t1.id
                  integer, username givespsswd id text);"), "failed");
               assert_res(cl->execute("COMMIT ANNOTATIONS;"), "issue when
                  creating tables");
               assert_res(cl->execute((string("INSERT INTO ") + PWD_TABLE_PREFIX
             + "users (username, psswd) VALUES ('alice',
                  'secretalice');")), "failed to log in user");
               assert_res(cl->execute((string("DELETE FROM ") + PWD_TABLE_PREFIX
             + "users  WHERE username = 'alice';")), "failed to
                  logout user");
               assert_res(cl->execute((string("INSERT INTO ") + PWD_TABLE_PREFIX
             + "users (username, psswd) VALUES ('alice',
                  'secretalice');")), "failed to log in user");
               assert_res(cl->execute("INSERT INTO users VALUES (1, 'alice');"),
                  "failed to add alice in users table");
               assert_res(cl->execute("INSERT INTO t1 VALUES (1, 'there you go',
                  23);"), "failed to insert");
               assert_res(cl->execute("SELECT * FROM t1;"), "failed");

               assert_res(cl->execute("SELECT post FROM t1 WHERE id = 1 AND age
                  = 23;"), "failed");
               assert_res(cl->execute("UPDATE t1 SET post = 'hello!' WHERE age >
                  22 AND id = 1;"), "failed");
               assert_res(cl->execute((string("INSERT INTO ") + PWD_TABLE_PREFIX
             + "users (username, psswd) VALUES ('raluca',
                  'secretraluca');")), "failed to log in user");
               assert_res(cl->execute("INSERT INTO users VALUES (2,
                  'raluca');"), "failed");
               assert_res(cl->execute("INSERT INTO t1 VALUES (2, 'my text',
                  5);"), "failed");
             

            //PRIVATE MESSAGES EXAMPLE
                cl->plain_execute("DROP TABLE IF EXISTS users, msgs,
               privmsg;");
                    assert_res(cl->execute("CREATE TABLE msgs (msgid equals
                       privmsg.msgid integer, msgtext encfor msgid text);"),
                       "failed");
                    assert_res(cl->execute("CREATE TABLE privmsg (msgid integer,
                       recid equals users.userid hasaccessto msgid integer,
                       senderid hasaccessto msgid integer);"), "failed");
                    assert_res(cl->execute("CREATE TABLE users (userid equals
                       privmsg.senderid integer, username givespsswd userid
                       text);"), "failed");
                    assert_res(cl->execute("COMMIT ANNOTATIONS;"), "issue when
                       creating tables");
                    assert_res(cl->execute((string("INSERT INTO ") +
                       PWD_TABLE_PREFIX + "users (username, psswd) VALUES
                       ('alice', 'secretalice');")), "failed to log in
                       user");
                    assert_res(cl->execute((string("INSERT INTO ") +
                       PWD_TABLE_PREFIX + "users (username, psswd) VALUES
                       ('bob', 'secretbob');")), "failed to log in
                       user");
                    assert_res(cl->execute("INSERT INTO users VALUES (1,
                       'alice');"), "failed");
                    assert_res(cl->execute("INSERT INTO users VALUES (2,
                       'bob');"), "failed");
                    assert_res(cl->execute("INSERT INTO privmsg (msgid, recid,
                       senderid) VALUES (9, 1, 2);"), "failed to send msg");
                    assert_res(cl->execute("INSERT INTO msgs  VALUES (1, 'hello
                       world');"), "failed to insert msg");
                    assert_res(cl->execute("SELECT msgtext from msgs WHERE msgid
                       = 1;"), "failed");
                    assert_res(cl->execute("SELECT msgtext from msgs, privmsg,
                       users WHERE username = 'alice' AND userid = recid AND
                       msgs.msgid = privmsg.msgid;"), "failed");
             
            //private messages without orphans
             cl->plain_execute("DROP TABLE IF EXISTS users, msgs,
               privmsg;");
               assert_res(cl->execute("CREATE TABLE msgs (msgid equals
                  privmsg.msgid integer, msgtext encfor msgid text);"),
                  "failed");
               assert_res(cl->execute("CREATE TABLE privmsg (msgid integer,
                  recid equals users.userid hasaccessto msgid integer,
                  senderid hasaccessto msgid integer);"), "failed");
               assert_res(cl->execute("CREATE TABLE users (userid equals
                  privmsg.senderid integer, username givespsswd userid
                  text);"), "failed");
               assert_res(cl->execute("COMMIT ANNOTATIONS;"), "issue when
                  creating tables");
               assert_res(cl->execute((string("INSERT INTO ") + PWD_TABLE_PREFIX
             + "users (username, psswd) VALUES ('alice',
                  'secretalice');")), "failed to log in user");
               assert_res(cl->execute((string("INSERT INTO ") + PWD_TABLE_PREFIX
             + "users (username, psswd) VALUES ('bob',
                  'secretbob');")), "failed to log in user");
               assert_res(cl->execute("INSERT INTO users VALUES (1, 'alice');"),
                  "failed");
               assert_res(cl->execute("INSERT INTO users VALUES (2, 'bob');"),
                  "failed");
               assert_res(cl->execute("INSERT INTO privmsg (msgid, recid,
                  senderid) VALUES (1, 1, 2);"), "failed to send msg");
               assert_res(cl->execute("INSERT INTO msgs  VALUES (1, 'hello
                  world');"), "failed to insert msg");
               assert_res(cl->execute("SELECT msgtext from msgs WHERE msgid =
                  1;"), "failed");
               assert_res(cl->execute("SELECT msgtext from msgs, privmsg, users
                  WHERE username = 'alice' AND userid = recid AND msgs.msgid =
                  privmsg.msgid;"), "failed");
             

            //USERID, GROUP, FORUM, SQL PRED EXAMPLE
            //    cl->plain_execute("DROP TABLE IF EXISTS users, usergroup,
            // groupforum, forum;");
            //    assert_res(cl->execute("CREATE TABLE users (userid integer,
            // username givespsswd userid text);"), "failed");
            //    assert_res(cl->execute("CREATE TABLE usergroup (userid equals
            // users.userid hasaccessto groupid integer, groupid integer);"),
            // "failed");
            //    assert_res(cl->execute("CREATE TABLE groupforum (forumid equals
            // forum.forumid integer, groupid equals usergroup.groupid
            // hasaccessto forumid if test(optionid) integer, optionid
            // integer);"), "failed");
            //    assert_res(cl->execute("CREATE TABLE forum (forumid integer,
            // forumtext encfor forumid text);"), "failed");
            //    assert_s(cl->plain_execute("DROP FUNCTION IF EXISTS test;"),
            // "failed");
            //    assert_s(cl->plain_execute("CREATE FUNCTION test (optionid
            // integer) RETURNS bool RETURN optionid=20;"), "failed");
            //
            //    //Alice is in group 1, Bob in group 2 and Chris is in group 1
            // and group 2
            //    //group 1 can see the forum text, group 2 cannot
            //    assert_res(cl->execute("INSERT INTO "psswdtable" VALUES
            // ('alice', 'secretalice');"), "failed to log in user");
            //    assert_res(cl->execute("INSERT INTO "psswdtable" VALUES ('bob',
            // 'secretbob');"), "failed to log in user");
            //    assert_res(cl->execute("INSERT INTO "psswdtable" VALUES
            // ('chris', 'secretbob');"), "failed to log in user");
            //
            //    assert_res(cl->execute("INSERT INTO users (username) VALUES
            // ('alice');"), "failed");
            //    assert_res(cl->execute("INSERT INTO users (username) VALUES
            // ('bob');"), "failed");
            //    assert_res(cl->execute("INSERT INTO users (username) VALUES
            // ('chris');"), "failed");
            //
            //    assert_res(cl->execute("INSERT INTO usergroup VALUES (1, 1);"),
            // "failed");
            //    assert_res(cl->execute("INSERT INTO usergroup VALUES (2, 2);"),
            // "failed");
            //    assert_res(cl->execute("INSERT INTO usergroup VALUES (3, 1);"),
            // "failed");
            //    assert_res(cl->execute("INSERT INTO usergroup VALUES (3, 2);"),
            // "failed");
            //
            //
            //    assert_res(cl->execute("INSERT INTO groupforum VALUES (1, 1,
            // 14);"), "failed");
            //    assert_res(cl->execute("INSERT INTO groupforum VALUES (1, 1,
            // 20);"), "failed");
            //    assert_res(cl->execute("INSERT INTO groupforum VALUES (1, 2,
            // 2);"), "failed");
            //    assert_res(cl->execute("INSERT INTO groupforum VALUES (1, 2,
            // 0);"), "failed");
            //
            //    assert_res(cl->execute("INSERT INTO forum (forumtext) VALUES
            // ('success--you can see forum text');"), "failed");
            //
            //    //all users log out, then each log in to have their
            // permissions tested
            //    assert_res(cl->execute("DELETE FROM "psswdtable" WHERE  username
            // = 'alice';"), "failed");
            //    assert_res(cl->execute("DELETE FROM "psswdtable" WHERE  username
            // = 'bob';"), "failed");
            //    assert_res(cl->execute("DELETE FROM "psswdtable" WHERE  username
            // = 'chris';"), "failed");
            //
            //
            //    assert_res(cl->execute("INSERT INTO "psswdtable" VALUES
            // ('alice', 'secretalice');"), "failed to log in user");
            //
            //    assert_res(cl->execute("SELECT forumtext from forum  WHERE
            // forumid  = 1;"), "Alice should succeed");
            //
            //    assert_res(cl->execute("DELETE FROM "psswdtable" WHERE  username
            // = 'alice';"), "failed");
            //    assert_res(cl->execute("INSERT INTO "psswdtable" VALUES ('bob',
            // 'secretbob');"), "failed to log in user");
            //
            //    assert_res(cl->execute("DELETE FROM "psswdtable" WHERE  username
            // = 'bob';"), "failed");
            //    assert_res(cl->execute("INSERT INTO "psswdtable" VALUES
            // ('chris', 'secretchris');"), "failed to log in user");
            //
            //    assert_res(cl->execute("SELECT forumtext from forum  WHERE
            // forumid  = 1;"), "chris should succeed");

            //multi-key debugging

                cl->plain_execute("DROP TABLE IF EXISTS hi, try, bye;");
               //some single key debugging
               assert_res(cl->execute("CREATE TABLE hi (id integer, age enc
                  integer, name enc text);"), "q failed");
               assert_res(cl->execute("INSERT INTO hi VALUES (3, 9,
                  'raluca');"), "q failed");
               assert_res(cl->execute("SELECT * FROM hi WHERE id = 4;"), "q
                  failed");

               assert_res(cl->execute("UPDATE hi SET age = age + 1, name = 'ana'
                  WHERE id = 3;"), "q failed");

               assert_res(cl->execute("CREATE TABLE try (id enc integer, age
                  integer);"), "q failed");
               assert_res(cl->execute("INSERT INTO try VALUES (5, 6);"), "q
                  failed");
               assert_res(cl->execute("SELECT u.*, v.* from hi u, try AS v;"),
                  "q failed");

               cl->outputOnionState();

               assert_res(cl->execute("SELECT * FROM (hi u, try AS v);"),"q
                  failed");

               assert_res(cl->execute("SELECT MAX(id) AS h, MIN(id) as ll FROM
                  try;"),"failed max");

               assert_res(cl->execute("SELECT u.id FROM try t LEFT JOIN try u ON
                  u.id = t.id;"),"");

               assert_res(cl->execute("SELECT * FROM try WHERE id = 5"),"");

               assert_res(cl->execute("SELECT * FROM try WHERE age IN
                  (0,9);"),"");


               assert_res(cl->execute("INSERT INTO try VALUES (3, 9), (5, 6),
                  (7,8);"), "");
               assert_res(cl->execute("SELECT * FROM try WHERE age in (6,8)
                  ORDER BY age ASC LIMIT 2;"),"");
               assert_res(cl->execute("SELECT * FROM try t WHERE t.age in (6,8)
                  ORDER BY id DESC LIMIT 2;"),"");

               assert_res(cl->execute("CREATE TABLE bye (id
                  integer);"),"failed");
               assert_res(cl->execute("INSERT INTO bye VALUES (3), (9),
                  (10);"),"failed");
               assert_res(cl->execute("SELECT * FROM bye WHERE id <>
                  3;"),"failed");

               assert_res(cl->execute("SELECT COUNT(id) AS my_count FROM
                  try;"),"failed");
               assert_res(cl->execute("SELECT COUNT(*) AS my_count  FROM
                  try;"),"failed");
               assert_res(cl->execute("SELECT count(distinct age) AS hello from
                  hi;"),"failed");

               assert_res(cl->execute("SELECT MAX(id) AS maximus, MIN(id) AS
                  minimus FROM bye WHERE id > 3;"),"failed");

               assert_res(cl->execute("INSERT INTO bye VALUES (3), (7), (13),
                  (43524)"),"failed");

               assert_res(cl->execute("SELECT id i FROM bye where id > 2 ORDER
                  BY i ASC LIMIT 5; "),"failed");

               assert_res(cl->execute("SELECT id i FROM bye WHERE id in
                  (7,13,3,4) AND (id in (7, 13)) ORDER BY i DESC LIMIT 9;
                  "),"failed");

               assert_res(cl->execute("INSERT INTO bye (id) VALUES (4), (6),
                  (-10)"),"failed");
               //assert_res(cl->execute(""),"failed");

               assert_res(cl->execute("DROP TABLE hi;"), "drop failed");
               assert_res(cl->execute("CREATE TABLE hi (id enc integer, name enc
                  text);"), "failed");
               assert_res(cl->execute("INSERT INTO  hi VALUES (3, 'ra'), (4,
                  'c');"), "failed");

               assert_res(cl->execute("SELECT * FROM  hi ORDER BY id;"), "order
                  by failed");

               cl->outputOnionState();
        } else {
            ResType r = cl->execute(cmd);
            if (r.ok && r.names.size() > 0) {
                std::vector<size_t> width(r.names.size());
                for (uint i = 0; i < r.names.size(); i++) {
                    width[i] = r.names[i].length();;
                    for (uint j = 0; j < r.rows.size(); j++)
                        if (r.rows[j][i].to_string().length() > width[i])
                            width[i] = r.rows[j][i].to_string().length();
                }
                {
                    stringstream ss;
                    for (unsigned int j = 0; j < r.names.size(); j++)
                        ss << left << setw((int) (width[j] + 2)) << r.names[j];
                    cout << ss.str() << endl;
                }
                for (unsigned int i = 0; i < r.rows.size(); i++) {
                    stringstream ss;
                    for (unsigned int j = 0; j < r.rows[i].size(); j++)
                        ss << left << setw((int) (width[j] + 2)) << r.rows[i][j].to_string();
                    cout << ss.str() << endl;
                }
            }
        }
    }

    delete cl;
    cout << "Goodbye!\n";
            */
}
/*
static void __attribute__((unused))
microEvaluate(const TestConfig &tc, int argc, char ** argv)
{
    cout << "\n\n Micro Eval \n------------------- \n \n";

    string masterKey =  BytesFromInt(mkey, AES_KEY_BYTES);
    EDBProxy * clsecure =
            new EDBProxy(tc.host, tc.user, tc.pass, tc.db);
    clsecure->setMasterKey(masterKey);
    EDBProxy * clplain = new EDBProxy(tc.host, tc.user, tc.pass,
            tc.db);

    clsecure->VERBOSE = false;
    clplain->VERBOSE = false;

    clsecure->execute("CREATE TABLE tableeval (id integer, age integer);");
    clplain->execute("CREATE TABLE tableeval (id integer, age integer);");

    int nrInsertsSecure = 500;
    int nrInsertsPlain = 1000;
    int nrSelectsPlain = 2000;
    int nrSelectsSecure = 3000;

    int networkOverhead = 10;     //10 ms latency per query

    startTimer();
    for (int i = 0; i < nrInsertsSecure; i++) {
        unsigned int val1 = rand() % 10;
        unsigned int val2 = rand() % 10;
        string qq = "INSERT INTO tableeval VALUES ( " + strFromVal(val1) +
                ", " + strFromVal(val2) + ");";
        clsecure->execute(qq);
    }
    double endTimer = (readTimer()/(1.0 * nrInsertsSecure));
    printf("secure insertion: no network %6.6f ms with network %6.6f ms \n",
            endTimer,
            endTimer+networkOverhead);

    startTimer();
    for (int i = 0; i < nrInsertsPlain; i++) {
        unsigned int val1 = rand() % 10;
        unsigned int val2 = rand() % 10;
        string qq = "INSERT INTO tableeval VALUES ( " + strFromVal(val1) +
                ", " + strFromVal(val2) + ");";
        clplain->execute(qq);
    }
    endTimer = (readTimer()/(1.0 * nrInsertsPlain));
    printf("plain insertion no network %6.6f ms with network %6.6f ms \n",
            endTimer,
            endTimer+networkOverhead);

    startTimer();
    for (int i = 0; i < nrSelectsSecure; i++) {
        unsigned int val1 = rand() % 50;
        string qq =
                "SELECT tableeval.id FROM tableeval WHERE tableeval.id = " +
                strFromVal(val1) + ";";
        clsecure->execute(qq);
        //unsigned int val2 = rand() % 50;
        //qq = "SELECT tableeval.age FROM tableeval WHERE tableeval.age > " +
        // strFromVal(val2) + ";";
        clsecure->execute(qq);
    }
    endTimer = (readTimer()/(1.0 * nrSelectsSecure));
    printf("secure selection no network %6.6f ms with network %6.6f ms \n",
            endTimer,
            endTimer+networkOverhead);

    startTimer();
    for (int i = 0; i < nrSelectsPlain; i++) {
        unsigned int val1 = rand() % 50;
        string qq =
                "SELECT tableeval.id FROM tableeval WHERE tableeval.id = " +
                strFromVal(val1) + ";";
        clplain->execute(qq);
        //unsigned int val2 = rand() % 50;
        //qq = "SELECT tableeval.age FROM tableeval WHERE tableeval.age > " +
        // strFromVal(val2) + ";";
        clplain->execute(qq);
    }
    endTimer = (readTimer()/(1.0 * nrSelectsPlain));
    printf("plain selection no network %6.6f ms with network %6.6f ms \n",
            endTimer,
            endTimer+networkOverhead);

    clsecure->execute("DROP TABLE tableeval;");
    clplain->execute("DROP TABLE tableeval;");

    delete clsecure;
    delete clplain;

}
*/
//at this point this function is mostly to figure our how binary data
// works..later will become a test
/*
   void tester::testMarshallBinary() {

    VERBOSE = true;

    execute("CREATE TABLE peoples (id bytea);");
    unsigned int len = 16;
    unsigned char * rBytes = randomBytes(len);
    cerr << " random Bytes are " << CryptoManager::marshallKey(rBytes) << "
       \n";
    cerr << " and marshalled they are " << marshallBinary(rBytes, len) <<
       "\n";
    string query = "INSERT INTO peoples VALUES ( " + marshallBinary(rBytes,
       len) + " );";
    execute(query);
    PGresult * res = execute("SELECT id, octet_length(id) FROM
       peoples;")->result;
    cout << "repr " << PQgetvalue(res, 0, 0) << "\n";
    execute("DROP TABLE peoples;");
   }
 */
 /*
//integration test
static void __attribute__((unused))
testEDBProxy(const TestConfig &tc)
{
    cout << "\n\n Integration Queries \n------------------- \n \n";

    string masterKey =  BytesFromInt(mkey, AES_KEY_BYTES);
    EDBProxy * cl = new EDBProxy(tc.host, tc.user, tc.pass, tc.db);
    cl->setMasterKey(masterKey);
    cl->VERBOSE = true;

    cl->execute("CREATE TABLE people (id integer, name text, age integer);");

    cl->execute("INSERT INTO people VALUES (34, 'raluca', 100);");
    cl->execute("INSERT INTO people VALUES (35, 'alice', 20);");
    cl->execute("INSERT INTO people VALUES (36, 'bob', 10);");

    cl->execute("SELECT people.id, people.age, people.name FROM people;");
    cl->execute("SELECT people.name FROM people WHERE people.age > 20;");
    cl->execute(
            "SELECT people.name, people.age FROM people WHERE people.name = 'alice' ;");

    cl->execute("DROP TABLE people;");

    delete cl;

    cout << "\n------------------- \n Integration test succeeded \n\n";
}
*/
static void
testPaillier(const TestConfig &tc, int ac, char **av)
{
    int noTests = 100;
    int nrTestsEval = 100;

    string masterKey = randomBytes(AES_KEY_BYTES);
    CryptoManager * cm = new CryptoManager(masterKey);

    for (int i = 0; i < noTests; i++) {
        int val = abs(rand() * 398493) % 12345;
        cerr << "Encrypt " << val << "\n";
        string ciph = cm->encrypt_Paillier(val);
        //myPrint(ciph, CryptoManager::Paillier_len_bytes);
        int dec = cm->decrypt_Paillier(ciph);
        //cerr << "\n decrypt to: " << dec << "\n";
        myassert(val == dec, "decrypted value is incorrect ");
    }

    string homCiph =
            homomorphicAdd(homomorphicAdd(cm->encrypt_Paillier(123),
                    cm->encrypt_Paillier(234),
                    cm->getPKInfo()),
                    cm->encrypt_Paillier(1001), cm->getPKInfo());
    myassert(cm->decrypt_Paillier(
            homCiph) == 1358, "homomorphic property fails! \n");
    cerr << "decrypt of hom " <<  cm->decrypt_Paillier(homCiph)  <<
            " success!! \n";

    cerr << "Test Paillier SUCCEEDED \n";

    cerr << "\n Benchmarking..\n";

    string ciphs[nrTestsEval];

    startTimer();
    for (int i = 0; i < noTests; i++) {
        int val = (i+1) * 10;
        ciphs[i] = cm->encrypt_Paillier(val);
    }
    double res = readTimer();
    cerr << "encryption takes " << res/noTests << " ms  \n";

    startTimer();
    for (int i = 0; i < noTests; i++) {
        int val = (i+1) * 10;

        int dec = cm->decrypt_Paillier(ciphs[i]);
        myassert(val == dec, "invalid decryption");
    }

    res = readTimer();
    cerr << "decryption takes " << res/noTests << " ms \n";
}

static void
testUtils(const TestConfig &tc, int ac, char **av)
{
    const char * query =
            "SELECT sum(1), name, age, year FROM debug WHERE debug.name = 'raluca ?*; ada' AND a+b=5 ORDER BY name;";

    LOG(test) << toString(parse(query, delimsStay, delimsGo, keepIntact), id_op);
}
/*
static void __attribute__((unused))
createTables(string file, EDBProxy * cl)
{
    ifstream createsFile(file);

    if (createsFile.is_open()) {
        while (!createsFile.eof()) {

            string query = "";
            string line = "";
            while ((!createsFile.eof()) &&
                    (line.find(';') == string::npos)) {
                createsFile >> line;
                query = query + line;
            }
            if (line.length() > 0) {
                cerr << query << "\n";
                if (!cl->execute(query).ok) {
                    cerr << "FAILED on query " << query << "\n";
                    createsFile.close();
                    delete cl;
                    exit(1);
                }
            }

        }
    } else {
        cerr << "error opening file " + file + "\n";
    }

    createsFile.close();

}
*/
static void __attribute__((unused))
convertQueries()
{

    ifstream firstfile("eval/tpcc/client.sql");
    ofstream secondfile("eval/tpcc/clientplain.sql");

    string line;

    int nr= 0;

    string transac = "";

    if (!firstfile.is_open()) {
        cerr << "cannot open input file \n";
        return;
    }

    if (!secondfile.is_open()) {
        cerr << "cannot open a second file \n";
        return;
    }

    while (!firstfile.eof() )
    {
        getline (firstfile,line);

        if (line.length() <= 1 ) {
            continue;
        }
        line = line + ";";

        //extract transaction number
        string no = line.substr(0, line.find('\t'));
        cerr << "no transac is " << no << "\n";
        line = line.substr(no.length()+1, line.length() - no.length()+1);

        if (no.compare(transac) != 0) {

            if (transac.length() > 0) {
                secondfile << "commit; \n" << "begin; \n";
            } else {
                secondfile << "begin; \n";
            }
            transac = no;
        }

        size_t index = 0;
        while (line.find(".", index) != string::npos) {
            size_t pos = line.find(".", index);
            index = pos+1;
            if (line[pos+1] >= '0' && line[pos+1] <= '9') {
                while ((line[pos]!=' ') && (line[pos] != ',') &&
                        (line[pos] != '\'')) {
                    line[pos] = ' ';
                    pos++;
                }
            }
        }

        while (line.find(">=") != string::npos) {
            size_t pos = line.find(">=");
            line[pos+1] = ' ';
            //TRACE SPECIFIC
            pos = pos + 3;
            int nr2 = 0;
            size_t oldpos = pos;
            while (line[pos] != ' ') {
                nr2 = nr2 * 10 + (line[pos]-'0');
                pos++;
            }
            nr2 = nr2 - 20;
            string replacement =  strFromVal((uint32_t)nr2) + "     ";
            line.replace(oldpos, 9, replacement);

        }
        while (line.find("<=") != string::npos) {
            size_t pos = line.find("<");
            line[pos+1] = ' ';
        }
        while (line.find(" -") != string::npos) {
            size_t pos = line.find(" -");
            line[pos+1] = ' ';
        }
        secondfile << line  << "\n";
        nr++;
    }

    cerr << "there are " << nr << " queries \n";
    firstfile.close();
    secondfile.close();

}
/*
static void __attribute__((unused))
test_train(const TestConfig &tc)
{

    cerr << "training \n";
    string masterKey =  BytesFromInt(mkey, AES_KEY_BYTES);

    EDBProxy * cl = new EDBProxy(tc.host, tc.user, tc.pass, tc.db);
    cl->setMasterKey(masterKey);

    cl->VERBOSE = true;
    cl->dropOnExit = true;

    cl->train("eval/tpcc/sqlTableCreates");
    cl->train("eval/tpcc/querypatterns.txt");
    cl->train("eval/tpcc/index.sql");
    cl->train_finish();
    cl->create_trained_instance();

    delete cl;

}
 */
/*
   void createIndexes(string indexF, EDBProxy * cl) {
        ifstream tracefile(indexF);

        string query;


        if (tracefile.is_open()) {
                while (!tracefile.eof()){

                        getline(tracefile, query);
                        if (query.length() > 1) {
                                edb_result * res =
                                   cl->execute(query);
                                if (res == NULL) {
                                        cerr << "FAILED on query " << query <<
                                           "\n";
                                        delete cl;
                                        tracefile.close();
                                        exit(1);
                                }
                        }

                }
        }

        tracefile.close();
   }

   void convertDump() {

        ifstream fileIn("eval/tpcc/dumpcustomer.sql");
        ofstream fileOut("eval/tpcc/dumpcustomer");

        string query;

        int index = 0;
        if (fileIn.is_open()) {
                        while (!fileIn.eof()){

                                getline(fileIn, query);
                                while (query.find("`")!=string::npos) {
                                        int pos = query.find("`");
                                        query[pos] = ' ';
                                }
                                if (query.length() > 1) {
                                        fileOut << query << "\n";
                                        index++;
                                }

                        }


    }

        cerr << "there are " << index << "entries \n";
        fileIn.close();
        fileOut.close();

   }

 */

/*
   const string createFile = "eval/tpcc/sqlTableCreates";
   const string queryTrainFile = "eval/tpcc/querypatterns.txt";
   const string dumpFile = "eval/tpcc/pieces/dump";
   const string queryFilePrefix = "eval/tpcc/";
   const string indexFile = "eval/tpcc/index.sql";
   const int nrDataToLoad = 100000; //how much the threads load in total
   const bool verbose = true;
   const bool isSecureLoad = true;
   const int logFrequency = 500;



   void runTrace(const char * suffix, int nrQueriesToRun, int isSecure) {

        struct timeval tvstart, tvend;

        unsigned char * masterKey =  BytesFromInt(mkey, AES_KEY_BYTES);
        EDBProxy * cl;

        if (isSecure) {
                cl = new EDBProxy("cryptdb", masterKey);
        } else {
                cl = new EDBProxy(tc.db);
        }
        cl->VERBOSE = verbose;

        cerr << "prepare \n";
        cl->nosubmit = true;
        createTables(createFile, cl);
        runForSteps(queryTrainFile, 100, cl);
        createIndexes(indexFile, cl);
        cl->nosubmit = false;

        gettimeofday(&tvstart, NULL);

        runForSteps(queryFilePrefix+suffix, nrQueriesToRun, cl);

        gettimeofday(&tvend,NULL);
        cerr << "all trace took " << ((tvend.tv_sec - tvstart.tv_sec)*1.0 +
           (tvend.tv_usec - tvstart.tv_usec)/1000000.0) << "s \n";


   }

 */

static string __attribute__((unused))
suffix(int no)
{
    int fi = 97 + (no/26);
    int se = 97 + (no%26);
    string first = string("") + (char)fi;
    string second = string("") + (char)se;
    string res = first + second;

    return res;
}
/*
   void tester::loadData(EDBProxy * cl, string workload, int logFreq) {

        ifstream dataFile(workload);
        ofstream outFile(workload+"answer");

        if ((!dataFile.is_open()) || (!outFile.is_open())) {
                cerr << "could not open file " << workload << "or the answer
                   file " << " \n";
                exit(1);
        }

        string query;

        int index = 0;
        while (!dataFile.eof()) {

                getline(dataFile, query);

                if (query.length() > 0) {
                        if (index % logFreq == 0) {
                                cerr << "load entry " << index << " from " <<
                                   workload << "\n" ;// query " << query <<
                                   "\n";
                        }
 */
/*edb_result * res = cl->execute(query);
                        if (index % logFreq== 0) {//cout << "executed\n";
                        }
                        if (res == NULL) {
                                cerr << workload + "offensive insert query "
                                   << query << "  \n";
                                while (res == NULL) {
                                        cerr << "retrying \n";
                                        sleep(10);
                                        res = cl->execute(query);
                                //delete cl;
                                //dataFile.close();
                                //exit(1);
                                }
                        }*//*
                        list<string> ress;
                        try {
                                ress =
                                   tcl->rewriteEncryptQuery(query);
                        } catch (CryptDBError se) {
                                cerr << "CryptDB error " << se.msg << "\n";
                                exit(1);
                        }
                        outFile << ress.front() << "\n";

                }

                index++;
        }

        outFile.close();
        dataFile.close();

        return;
   }



   bool isbegin(string s) {
        if (s.compare("begin;") == 0){
                return true;
        }
        if (s.compare("begin; ") == 0){
                        return true;
        }
        return false;
   }
   bool iscommit(string s) {
        if (s.compare("commit;") == 0){
                return true;
        }
        if (s.compare("commit; ") == 0){
                        return true;
        }
        return false;
   }

   void runTrace(EDBProxy * cl, int logFreq, string workload, bool isSecure,
      bool hasTransac,
                int & okinstrCount, int & oktranCount, int & totalInstr, int &
                   totalTran) {

        ifstream tracefile(workload);

        string query;

        if (!tracefile.is_open()) {
                cerr << "cannot open " << workload << "\n";
        }

        int count = 0;//nr of failed instructions

        int index = 0; //the number of query execute currently
        //counts for the number of instructions or transactions succesfully
           run
        okinstrCount = 0;
    oktranCount = 0;
    totalTran = 0;
    totalInstr = 0;

    bool tranAborted = false;

    while (!tracefile.eof()) {


                getline(tracefile, query);

                if (query.length() < 1) {
                        continue;
                }

                index++;
                if (index % logFreq     == 0) {cerr << workload << " " <<
                   index << "\n";}

                if (!hasTransac) {
                        if (isSecure) {
                                edb_result * res =
                                   cl->execute(query);
                                if (res == NULL) {
                                        count++;
                                }
                        } else {
                                PGresult * res =
                                   cl->plain_execute(query);
                                if (!((PQresultStatus(res) == PGRES_TUPLES_OK)
 || (PQresultStatus(res) ==
                                   PGRES_COMMAND_OK))) {
                                        count++;
                                }
                        }
                } else {

                                if (isbegin(query)) {
                                        tranAborted = false;
                                        totalTran++;
                                }
                                if (tranAborted) {
                                                continue;
                                }

                                bool instrOK = true;
                                if (isSecure) {
                                        edb_result * res =
                                           cl->execute(query);
                                        if (res == NULL) {
                                                instrOK = false;
                                        }
                                } else {
                                        PGresult * res =
                                           cl->plain_execute(query);
                                        if (!((PQresultStatus(res) ==
                                           PGRES_TUPLES_OK) ||
                                           (PQresultStatus(res) ==
                                           PGRES_COMMAND_OK))) {
                                                instrOK = false;
                                        }
                                }

                                if (!instrOK) {
                                        PGresult * res =
                                           cl->plain_execute("abort;");
                                        if (!((PQresultStatus(res) ==
                                           PGRES_TUPLES_OK) ||
                                           (PQresultStatus(res) ==
                                           PGRES_COMMAND_OK))) {
                                                                cerr <<
                                                                   workload <<
                                                                   ":
                                                                   returning
                                                                   bad status
                                                                   even on
                                                                   abort,
                                                                   exiting ;";
                                                                tracefile.close();
                                                                exit(1);
                                        }
                                        tranAborted = true;
                                        cerr << "aborting curr tran \n";
                                } else {
                                        if (iscommit(query)) {
                                                oktranCount++;
                                                //cerr << "oktranCount becomes
                                                   " << oktranCount << " \n";
                                        }

                                    okinstrCount++;

                                }
                }



        }

        totalInstr = index;
        tracefile.close();



        return;
   }


   void simpleThroughput(int noWorkers, int noRepeats, int logFreq, int
      totalLines, string dfile, bool isSecure, bool hasTransac) {
        cerr << "throughput benchmark \n";
                         */
//    int res = system("rm eval/pieces/*");
/*
        ifstream infile(dfile);

        int linesPerWorker = totalLines / noWorkers;

        string query;

        if (!infile.is_open()) {
                cerr << "cannot open " + dfile << "\n";
                exit(1);
        }

        //prepare files
        for (int i = 0; i < noWorkers; i++) {
                string workload = string("eval/pieces/piece") + suffix(i);
                ofstream outfile(workload);

                if (!outfile.is_open()) {
                        cerr << "cannot open file " << workload << "\n";
                        infile.close();
                        exit(1);
                }

                getline(infile, query);

                if (hasTransac && (!isbegin(query))) {
                        outfile << "begin; \n";
                }

                for (int j = 0; j < linesPerWorker; j++) {
                        outfile << query << "\n";
                        if (j < linesPerWorker-1) {getline(infile, query);}
                }

                if (hasTransac && (!iscommit(query))) {
                        outfile << "commit; \n";
                }

                outfile.close();

                //we need to concatenate the outfile with itself noRepeats
                   times
                res = system("touch temp;");
                for (int j = 0; j < noRepeats; j++) {
                        res = system(string("cat temp ") + workload +
                           string(" > temp2;"));
                        res = system("mv temp2 temp");
                }
                res = system("mv temp " + workload);

        }

        infile.close();

    res = system("rm eval/pieces/result;");
    res = system("touch eval/pieces/result;");

    ofstream resultFile("eval/pieces/result");
    ifstream resultFileIn;

    if (!resultFile.is_open()) {
        cerr << "cannot open result file \n";
        exit(1);
    }

        timeval starttime, endtime;

        int childstatus;
    int index;
    int i;
    pid_t pids[noWorkers];

        double interval, querytput, querylat, trantput, tranlat;
        int allInstr, allInstrOK, allTran, allTranOK;

        for (i = 0; i < noWorkers; i++) {
                index = i;
                pid_t pid = fork();
                if (pid == 0) {
                        goto dowork;
                } else if (pid < 0) {
                        cerr << "failed to fork \n";
                        exit(1);
                } else { // in parent
                        pids[i] = pid;
                }
        }

        //parent
        for (i = 0; i < noWorkers; i++) {

                if (waitpid(pids[i], &childstatus, 0) == -1) {
                        cerr << "there were problems with process " << pids[i]
                           << "\n";
                }

        }

        resultFile.close();

        resultFileIn.open("eval/pieces/result", ifstream::in);

        if (!resultFileIn.is_open()) {
                cerr << "cannot open results file to read\n";
                exit(1);
        }

        querytput = 0; querylat = 0; trantput = 0; tranlat = 0;
        allInstr = 0; allInstrOK = 0; allTran = 0; allTranOK = 0;

        for (i = 0; i < noWorkers; i++) {

                double currquerytput, currquerylat, currtrantput, currtranlat;
                int currtotalInstr, currokInstr, currtotalTran, currokTran;

                if (!hasTransac) {
                        resultFileIn >> index; resultFileIn >> interval;
                           resultFileIn >> currquerytput; resultFileIn >>
                           currquerylat;
                        cerr << index << " " << interval << " sec " <<
                           currquerytput << " queries/sec " << currquerylat <<
                           " secs/query \n";
                        querytput = querytput + currquerytput;
                        querylat = querylat + currquerylat; }
                else {
                        resultFileIn >> index;
                        resultFileIn >> interval; resultFileIn >>
                            currtrantput; resultFileIn >> currquerytput;
                           resultFileIn >> currtranlat; resultFileIn >>
                           currquerylat; resultFileIn >> currtotalInstr;
                           resultFileIn >> currokInstr; resultFileIn >>
                           currtotalTran; resultFileIn >> currokTran;
                        querytput +=currquerytput;
                        querylat +=currquerylat;
                        trantput += currtrantput;
                        tranlat += currtranlat;
                        allInstr += currtotalInstr;
                        allInstrOK += currokInstr;
                        allTran += currtotalTran;
                        allTranOK += currokTran;

                        cerr << "worker " << i << " interval " << interval <<
                           " okInstr " <<  currokInstr << " okTranCount " <<
                           currokTran << " totalInstr " << currtotalInstr << "
                           totalTran " <<  currtotalTran << "\n";

                }
        }

        if (!hasTransac) {
                cerr <<"overall:  throughput " << querytput << " queries/sec
                   latency " << querylat/noWorkers << " sec/query \n";
        } else {
                querylat = querylat / noWorkers;
                tranlat  = tranlat / noWorkers;
                if (isSecure) {
                        cerr << "secure: ";
                } else {
                        cerr << "plain: ";
                }
                cerr << " querytput " << querytput << " querylat " << querylat
                   << " trantput " << trantput << " tranlat " << tranlat << "
                   allInstr " << allInstr << " allInstrOK " << allInstrOK << "
                   tran failed" << allTran-allTranOK << " allTransacOK " <<
                   allTranOK << " \n";


        }
        resultFileIn.close();

        return;


        //children:
        dowork:

        EDBProxy * cl;

        if (isSecure) {
                unsigned char * masterKey =  BytesFromInt(mkey,
                   AES_KEY_BYTES);

        cl = new EDBProxy("cryptdb", masterKey);

                cl->train("schema");
                cl->train("queries");
                cl->train_finish();


        } else {
                 cl = new EDBProxy(tc.db);
        }

        string workload = string("eval/pieces/piece") + suffix(index);
        //execute on the workload
        cerr << "in child workload file <" << workload << "> \n";
        cerr << "value of index is " << index << "\n";

        int okInstrCount, okTranCount, totalInstr, totalTran;


        gettimeofday(&starttime, NULL);

        runTrace(cl, logFreq, workload, isSecure, hasTransac, okInstrCount,
           okTranCount, totalInstr, totalTran);
        //now we need to start the workers
        gettimeofday(&endtime, NULL);


        interval = 1.0 * timeInSec(starttime, endtime);

        if (!hasTransac) {
                querytput = linesPerWorker *  noRepeats * 1.0 / interval;
                querylat = interval / (linesPerWorker * noRepeats);

                resultFile << index << " " << interval << " " << querytput <<
                   " " << querylat << "\n";
        } else { //report  workerid  timeinterval  trantput querytput tranlate
           querylate totalInstr okInstr totalTRan okTran
                myassert(noRepeats == 1, "repeats more than one,transactions
                   fail automatically\n");
                double trantput = okTranCount * 1.0 / interval;
                double querytput = okInstrCount*1.0/interval;
                double tranlat = interval*1.0/okTranCount;
                double querylat = interval*1.0/okInstrCount;
                resultFile << index << " " << interval << " " << trantput << "
                   " << querytput << " " << tranlat << " " << querylat << " "
                   << totalInstr << " " << okInstrCount << " " << totalTran <<
                   " " << okTranCount << "\n";
        }

        delete cl;

        return;
   }

   void createInstance() {
        unsigned char * masterKey =  BytesFromInt(mkey, AES_KEY_BYTES);

        EDBProxy * cl = new EDBProxy("cryptdb", masterKey);

        cl->train("eval/tpcc/sqlTableCreates");
        cl->train("eval/tpcc/querypatterns.txt");
        cl->train("eval/tpcc/index.sql");
        cl->train_finish();

        cl->create_trained_instance(true);

        EDBProxy * plaincl = new EDBProxy(tc.db);

        int res = system("psql < eval/tpcc/sqlTableCreates");
        res = system("psql < eval/tpcc/index.sql");

        delete cl;
        delete plaincl;

   }

   void parallelLoad(int noWorkers, int totalLines, int logFreq,  string
      dfile, int workeri1, int workeri2) {


        cerr << "Parallel loading from " << dfile << ". \n";

        unsigned char * masterKey =  BytesFromInt(mkey, AES_KEY_BYTES);

        tester t = tester("cryptdb", masterKey);

        int res = system("mkdir eval/pieces;");
 */
//    res = system("rm eval/pieces/*");
/*
        string splitComm = "split  -l " +
           strFromVal((uint32_t)(totalLines/noWorkers)) + " -a 2 " + dfile +
           " eval/pieces/piece";
        cerr << "split comm " << splitComm << "\n";
        res  = system(splitComm);
        myassert(res == 0, "split failed");

        EDBProxy * cl = new EDBProxy("cryptdb", masterKey);

        cl->train("eval/tpcc/sqlTableCreates");
        cl->train("eval/tpcc/querypatterns.txt");
        cl->train("eval/tpcc/index.sql");
        cl->train_finish();


        cl->create_trained_instance(false);

        int index = 0;
        for (int i = workeri1; i <= workeri2; i++) {
                index = i;
                pid_t pid = fork();
                if (pid == 0) {
                        goto dowork;
                } else if (pid < 0) {
                        cerr << "failed to fork \n";
                        exit(1);
                }
        }

        //parent
        return;

        dowork:
        string workload = string("eval/pieces/piece") + suffix(index);
        //execute on the workload
        cerr << "in child workload file <" << workload << "> \n";
        cerr << "value of index is " << index << "\n";
        t.loadData(cl, workload, logFreq);
   }


   void executeQueries(EDBProxy * cl, string workload, string resultFile, int
      timeInSecs, int logFreq) {
        ifstream tracefile(workload);
        string query;
        struct timeval tvstart, tvend;

        if (!tracefile.is_open()) {
                cerr << "cannot open " << workload << "\n";
                exit(1);
        }

        gettimeofday(&tvstart, NULL);
        gettimeofday(&tvend,NULL);

        int index = 0;

        while (timeInSec(tvstart, tvend) < timeInSecs) {
                while (!tracefile.eof()) {
                        getline(tracefile, query);
                        edb_result * res = cl->execute(query);
                        index++;
                        if (index % logFreq == 0) {cerr << index << "\n";}
                        if (res == NULL) {
                                cerr << "FAILED on query " << query << "\n";
                                cerr << "query no " << index << "\n";
                                delete cl;
                                tracefile.close();
                                exit(1);
                        }
                        if (index % 100 == 0) {
                                gettimeofday(&tvend, NULL);
                                if (timeInSec(tvstart, tvend) >= timeInSecs) {
                                        goto wrapup;
                                }
                        }

                }

                gettimeofday(&tvend,NULL);
        }

        wrapup:
        tracefile.close();
        ofstream resFile(getCStr(resultFile));
        if (!resFile.is_open()) {
                cerr << "cannot open result file " << resultFile << "\n";
                exit(-1);
        }
        resFile << index << "\n";
        resFile << timeInSec(tvstart, tvend) << "\n";
        resFile.close();
   }


   void throughput(int noClients, int totalLines, int timeInSecs, int logFreq,
      string queryFile, bool isSecure) {
        //prepare files

        int res = system("mkdir eval/queries;");
 */
//    res = system("rm eval/queries/*");
/*
        string splitComm = "split  -l " +
           strFromVal((uint32_t)(totalLines/noClients)) + " -a 1 " +
           queryFile + " eval/queries/piece";
        cerr << "split comm " << splitComm << "\n";
        res  = system(getCStr(splitComm));
        myassert(res == 0, "split failed");

        EDBProxy * cl;

        if (isSecure) {

                unsigned char * masterKey =  BytesFromInt(mkey,
                   AES_KEY_BYTES);

                cl = new EDBProxy("cryptdb", masterKey);

                cl->train("eval/tpcc/sqlTableCreates");
                cl->train("eval/tpcc/querypatterns.txt");
                cl->train("eval/tpcc/index.sql");
                cl->train_finish();

        }
        else {
                cl = new EDBProxy(tc.db);
        }

        int index = 0;
        pid_t pids[noClients];
        string resultFile[noClients];
        int i, childstatus;
        pid_t pid;
        double throughput;

        for (i = 0; i < noClients; i++) {
                resultFile[i] = "eval/queries/answer" + (char)('a'+i);
                index = i;
                pid = fork();
                if (pid == 0) {
                        goto dowork;
                } else if (pid < 0) {
                        cerr << "failed to fork \n";
                        exit(1);
                } else {
                        //parent
                        pids[i] = pid;
                }
        }


        //parents
        for (i = 0; i < noClients; i++) {

                if (waitpid(pids[i], &childstatus, 0) == -1) {
                        cerr << "there were problems with process " << pids[i]
                           << "\n";
                }
        }

        //collect results and compute throughput
        throughput = 0;
        for (int i = 0; i < noClients; i++) {
                string resfile = string("eval/queries/answer")+char('a'+i);
                ifstream result(getCStr(resfile));
                int count;
                double secs;
                result >> count;
                result >> secs;
                cerr << "worker i processed " << count << " in " << secs << "
                   secs \n";
                throughput += (count*1.0/secs);
        }

        cerr << "overall throughput " << throughput << "\n";

        return;

        dowork: //child
        string workload = string("eval/queries/piece") + (char)('a' + index);
        //execute on the workload
        cerr << "in child workload file <" << workload << "> \n";
        cerr << "value of index is " << index << "\n";
        executeQueries(cl, workload, resultFile[index], timeInSecs, logFreq);


        return;



   }

   void parse() {
        ifstream f("queries");
        ofstream o("queries2");
        string q;

        while (!f.eof()) {
                getline(f, q);

                if (q.length() > 0) {
                        o << q << ";\n";
                }
        }

        f.close();
        o.close();
   }

   void simple() {

        unsigned char * masterKey =  BytesFromInt(mkey, AES_KEY_BYTES);

        EDBProxy * cl = new EDBProxy("cryptdb", masterKey);

        cl->train("schema");
        cerr << "done with creates \n";

        cl->train("queries");
        cl->train_finish();

   //    cl->create_trained_instance();

        ifstream f("insertslast");

        if (!f.is_open()) {
                cerr << "cannot open f \n";
                exit(1);
        }

        cl->ALLOW_FAILURES = true;

        int log = 2000;
        int index = 0;
        string q;
        while (!f.eof()) {
                getline(f, q);
                index++;
                if (index % log == 0) {cerr << index << "\n";}
                if (q.length() == 0) {
                        continue;
                }


                cl->execute(q);
        }

        cerr << "done \n";
        return;


   }

   void latency(string queryFile, int maxQueries, int logFreq, bool isSecure,
      int isVerbose) {
 */
/*ofstream outputnew("cleanquery"); //DO

        if (!outputnew.is_open()) { //DO
                cerr << "cannot open cleanquery file \n";
                exit(1);
        }
 */
/*    cerr << "starting \n";
        struct timeval tvstart;
        struct timeval tvend;

        EDBProxy * cl;

        if (isSecure) {

                unsigned char * masterKey =  BytesFromInt(mkey,
                   AES_KEY_BYTES);

                cl = new EDBProxy("cryptdb", masterKey);

                if (isVerbose) {
                                cl->VERBOSE = true;
                } else {
                                cl->VERBOSE = false;
                }


                cl->train("eval/tpcc/sqlTableCreates");
                cerr << "done with creates \n";

                cl->train("eval/tpcc/querypatterns.txt");


                cl->train_finish();

                cerr << "done \n";
                return;

        }
        else {
                cl = new EDBProxy(tc.db);


        }

        if (isVerbose) {
                cl->VERBOSE = true;
        } else {
                cl->VERBOSE = false;
        }

        ifstream tracefile(getCStr(queryFile));

        if (!tracefile.is_open()) {
                cerr << "cannot open file " << queryFile << "\n";
                exit(-1);
        }

        gettimeofday(&tvstart, NULL);

        string query;

        int index = 0;
        while (!tracefile.eof()) {
                getline(tracefile, query);
                if (query.size() == 0) {
                        continue;
                }
 */    /*    try {
                        PGresult * res =
   (*cl)->plain_execute(query);//DO
                        ExecStatusType est = PQresultStatus(res);
                        if ((est == PGRES_COMMAND_OK) || (est ==
   (*PGRES_TUPLES_OK))) {
                          outputnew << query << "\n";
                        } else {
                                cl->plain_execute("abort;");
                        }
  */
/*            cl->rewriteEncryptQuery(query);

                        //cerr << query << "\n";
                        //cerr << resQuery.front() << "\n";
                }  catch (CryptDBError se) {
                        cerr << se.msg << "\n aborting \n";
                        return;
                }

                if (index % logFreq == 0) {cerr << index << "\n";}

                index++;
                if (index == maxQueries) {
                        break;
                }

        }

        //outputnew.close(); //DO

        gettimeofday(&tvend,NULL);
        tracefile.close();

        cerr << "file " << queryFile << " overall took " << timeInSec(tvstart,
           tvend) << " each statement took " << (timeInSec(tvstart,
           tvend)/index*1.0)*1000.0  << " ms \n";

   }
 */
/*
static void
encryptionTablesTest(const TestConfig &tc, int ac, char **av)
{
    EDBProxy * cl =
            new EDBProxy(tc.host, tc.user, tc.pass, tc.db);
    cl->setMasterKey(randomBytes(AES_KEY_BYTES));

    int noHOM = 100;
    int noOPE = 100;

    cl->VERBOSE = true;

    if (!cl->execute("CREATE TABLE try (age integer);").ok)
        return;

    struct timeval starttime, endtime;

    gettimeofday(&starttime, NULL);
    cl->createEncryptionTables(noHOM, noOPE);
    gettimeofday(&endtime, NULL);

    cerr << "time per op" <<
            timeInSec(starttime, endtime)*1000.0/(noHOM+noOPE) << "\n";

    if (!cl->execute("INSERT INTO try VALUES (4);").ok) return;
    if (!cl->execute("INSERT INTO try VALUES (5);").ok) return;
    if (!cl->execute("INSERT INTO try VALUES (5);").ok) return;
    if (!cl->execute("INSERT INTO try VALUES (5);").ok) return;
    if (!cl->execute("INSERT INTO try VALUES (5);").ok) return;
    if (!cl->execute("INSERT INTO try VALUES (5);").ok) return;
    if (!cl->execute("INSERT INTO try VALUES (5);").ok) return;
    if (!cl->execute("INSERT INTO try VALUES (10000001);").ok) return;
    if (!cl->execute("SELECT age FROM try WHERE age > 1000000;").ok) return;
    if (!cl->execute("DROP TABLE try;").ok) return;
    delete cl;
}
*/
static void
testParseAccess(const TestConfig &tc, int ac, char **av)
{
    cerr << "testParseAccess uses old EDBProxy -- not run" << endl;
    /*
    EDBProxy * cl = new EDBProxy(tc.host, tc.user, tc.pass, tc.db);
    cl->setMasterKey(BytesFromInt(mkey, AES_KEY_BYTES));

    cl->VERBOSE = true;
    cl->execute(
            "CREATE TABLE test (gid integer, t text encfor gid, f integer encfor mid, mid integer);");

    string q = "INSERT INTO test VALUES (3, 'ra', 5, 4);";
    cerr << q;
    list<string> query = parse(q, delimsStay, delimsGo, keepIntact);

    TMKM tmkm;
    QueryMeta qm;

    assert_s(false, "test no longer valid because of interface change ");

    //cl->getEncForFromFilter(INSERT, query, tmkm, qm); <-- no longer valid

    map<string, string>::iterator it = tmkm.encForVal.begin();

    while (it != tmkm.encForVal.end()) {
        cout << it->first << " " << it->second << "\n";
        it++;
    }

    q = "SELECT * FROM test WHERE gid = 3 AND mid > 4 AND t = 'ra';";
    cerr << q;
    query = parse(q, delimsStay, delimsGo, keepIntact);

    //cl->getEncForFromFilter(SELECT, query, tmkm, qm); <-- no longer valid
    // due to interface change

    it = tmkm.encForVal.begin();

    while (it != tmkm.encForVal.end()) {
        cout << it->first << " " << it->second << "\n";
        it++;
    }
    cl->execute("DROP TABLE test;");
    */
}

static void
autoIncTest(const TestConfig &tc, int ac, char **av)
{
    /*
    string masterKey = BytesFromInt(mkey, AES_KEY_BYTES);
    string host = tc.host;
    string user = tc.user;
    string db = tc.db;
    string pwd = tc.pass;
    cerr << "connecting to host " << host << " user " << user << " pwd " <<
            pwd << " db " << db << endl;
    EDBProxy * cl = new EDBProxy(host, user, pwd, db);
    cl->setMasterKey(masterKey);
    cl->VERBOSE = true;

    cl->plain_execute("DROP TABLE IF EXISTS t1, users;");
    assert_res(cl->execute(
            "CREATE TABLE t1 (id integer, post encfor id det text, age encfor id ope bigint);"),
            "failed");
    assert_res(cl->execute(
            "CREATE TABLE users (id equals t1.id integer, username givespsswd id text);"),
            "failed");
    assert_res(cl->execute(
            "INSERT INTO " psswdtable
            " VALUES ('alice', 'secretalice');"),
            "failed to log in user");
    assert_res(cl->execute(
            "DELETE FROM " psswdtable
            " WHERE username = 'al\\'ice';"),
            "failed to logout user");
    assert_res(cl->execute(
            "INSERT INTO " psswdtable
            " VALUES ('alice', 'secretalice');"),
            "failed to log in user");
    assert_res(cl->execute(
            "INSERT INTO users VALUES (1, 'alice');"),
            "failed to add alice in users table");
    ResType rt = cl->execute(
            "INSERT INTO t1 (post, age) VALUES ('A you go', 23);");
    assert_s(rt.ok && rt.names[0].compare("cryptdb_autoinc") == 0, "fieldname is not autoinc");
    assert_s(rt.ok && rt.rows[0][0].data == "1", "autoinc not correct1");

    rt = cl->execute(
            "INSERT INTO t1 (post, age) VALUES ('B there you go', 23);");
    assert_s(rt.ok && rt.rows[1][0].data == "2", "autoinc not correct2");

    rt = cl->execute("INSERT INTO t1 VALUES (3, 'C there you go', 23);");
    cerr << "result is  " << rt.rows[0][0].to_string() << "\n";
    assert_s(rt.ok && rt.rows[0][0].data == "3", "autoinc not correct3");

    rt = cl->execute(
            "INSERT INTO t1 (post, age) VALUES ( 'D there you go', 23);");
    assert_s(rt.ok && rt.rows[0][0].data == "4", "autoinc not correct4");
    */
    //delete cl;
}

static void
accessManagerTest(const TestConfig &tc, int ac, char **av)
{

    /*cerr << "============================= Consolidate
       ========================" << endl;
       assert_s(isOnly("10",math,noMath), "10 is apparently not math");
       assert_s(isOnly("8-3",math,noMath), "8-3 is apparently not math");
       assert_s(isOnly("(1+2)/3*4.5-7",math,noMath), "longer equation is not
          math");

       string basic[] = {"WHERE","a",
          "=","8-3","AND","b","=","10","+","2","AND","c=","a+2","AND","x=3+6"};
       string basic_r[] = {"WHERE","a",
          "=","5","AND","b","=","12","AND","c=","a+2","AND","x=3+6"};
       list<string> word_list (basic, basic + sizeof(basic)/sizeof(int));
       list<string> word_res (basic_r, basic_r + sizeof(basic_r)/sizeof(int));
       string select[] = {"SELECT", "post", "FROM", "t1", "WHERE", "id", "=",
          "1", "AND", "age", "=", "23"};
       list<string> select_list(select, select + sizeof(select)/sizeof(int));
       list<string> select_res(select, select + sizeof(select)/sizeof(int));
       string limit[] = {"SELECT", "post", "FROM", "t1", "WHERE", "id", "=",
          "1", "AND", "age", "=", "23", "LIMIT", "5"};
       list<string> limit_list(limit, limit + sizeof(limit)/sizeof(int));
       list<string> limit_res(limit, limit + sizeof(limit)/sizeof(int));
       string insert2[] = {"INSERT", "INTO", "t1", "VALUES", "(", "3+5", ",",
          "23", ",", "43-34",")"};
       string insert_r[] = {"INSERT", "INTO", "t1", "VALUES", "(", "8", ",",
          "23", ",", "9",")"};
       list<string> insert_list(insert2, insert2 +
          sizeof(insert2)/sizeof(int));
       list<string> insert_res(insert_r, insert_r +
          sizeof(insert_r)/sizeof(int));
       string update[] = {"UPDATE", "t1", "WHERE", "field2", "=", "'", "14",
          "'", "SET", "a=a", "+", "9", "-8"};
       string update_r[] = {"UPDATE", "t1", "WHERE", "field2", "=", "'", "14",
          "'", "SET", "a=a", "+", "1"};
       list<string> update_list(update, update + sizeof(update)/sizeof(int));
       list<string> update_res(update_r, update_r +
          sizeof(update_r)/sizeof(int));

       vector<list<string> > input;
       input.push_back(word_list);
       input.push_back(select_list);
       input.push_back(limit_list);
       input.push_back(insert_list);
       input.push_back(update_list);
       vector<list<string> > res;
       res.push_back(word_list);
       res.push_back(select_list);
       res.push_back(limit_list);
       res.push_back(insert_list);
       res.push_back(update_list);

       vector<list<string> >::iterator inp = input.begin();
       vector<list<string> >::iterator re = res.begin();
       list<string>::iterator in;
       list<string>::iterator r;
       for(inp = input.begin(); inp != input.end(); inp++, re++) {
            consolidate(*inp);
            assert_s(inp->size() == re->size(), "result and expected results
               are different sizes");
            in = inp->begin();
            r = re->begin();
            for(in = inp->begin(); in != inp->end(); in++, r++) {
                    assert_s(in->compare(*r) == 0, "result and expected
                       results disagree");
            }
       }

       return;*/

    //Testing new AccessManager, called AccessManager2
    /*cerr <<
            "============================= AccessManager2 =================================="
            << endl;
    MetaAccess * meta;
    meta = new MetaAccess(new Connect(tc.host, tc.user, tc.pass,
            tc.db),true);

    meta->addEquals("u.uid","g.uid");
    meta->addAccess("u.uid","g.gid");

    assert_s(!meta->CheckAccess(), "passes access check with no givesPsswd");

    meta->addGives("u.uname");

    assert_s(
            !meta->CheckAccess(), "passes access check with broken access tree");

    KeyAccess * am;
    am = new KeyAccess(new Connect(tc.host,tc.user,tc.pass,tc.db));

    am->addEquals("u.uid","g.uid");     //1
    am->addAccess("u.uname","u.uid");
    am->addEquals("m.uid","u.uid");     //3
    am->addAccess("m.uid","m.mess");
    am->addAccess("u.uid","u.acc");
    am->addAccess("g.uid","g.gid");
    am->addEquals("g.gid","x.gid");     //2
    am->addAccess("f.gid","f.fid");
    am->addAccess("x.gid","x.mailing_list");
    am->addEquals("g.gid","f.gid");     //4
    am->addAccess("m.mess","m.sub");
    am->addAccess("f.gid","u.acc");

    am->addGives("u.uname");

    am->addAccess("msgs.msgid", "msgs.msgtext");

    am->addEquals("msgs.msgid","privmsgs.msgid");
    am->addEquals("privmsgs.recid", "users.userid");

    am->addAccess("privmsgs.recid", "privmsgs.msgid");
    am->addAccess("privmsgs.senderid", "privmsgs.msgid");

    am->addEquals("users.userid", "privmsgs.senderid");

    am->addGives("users.username");
    am->addAccess("users.username", "users.userid");

    cerr << "\n";

    std::set<string> generic_gid = am->getEquals("g.gid");
    assert_s(generic_gid.find(
            "f.gid") != generic_gid.end(), "f.gid is not equal to g.gid");
    assert_s(generic_gid.find(
            "x.gid") != generic_gid.end(), "x.gid is not equal to g.gid");

    std::set<string> generic_uid = am->getEquals("m.uid");
    assert_s(generic_uid.find(
            "u.uid") != generic_uid.end(), "u.uid is not equal to m.uid");
    assert_s(generic_uid.find(
            "g.uid") != generic_uid.end(), "g.uid is not equal to m.uid");
    assert_s(generic_uid.find(
            "f.gid") == generic_uid.end(), "m.uid is equal to f.gid");

    std::set<string> gid_hasAccessTo = am->getTypesHasAccessTo("g.gid");
    assert_s(gid_hasAccessTo.find(
            "f.fid") != gid_hasAccessTo.end(),
            "g.gid does not have access to f.fid");
    assert_s(gid_hasAccessTo.find(
            "x.mailing_list") != gid_hasAccessTo.end(),
            "g.gid does not have access to x.mailing_list");
    assert_s(gid_hasAccessTo.find(
            "g.uid") == gid_hasAccessTo.end(),
            "g.gid does have access to g.uid");
    assert_s(gid_hasAccessTo.find(
            "f.gid") == gid_hasAccessTo.end(),
            "getTypesHasAccessTo(g.gid) includes f.gid");
    assert_s(gid_hasAccessTo.find(
            "g.gid") == gid_hasAccessTo.end(),
            "getTypesHasAccessTo(g.gid) includes g.gid");

    std::set<string> mess_accessibleFrom = am->getTypesAccessibleFrom(
            "m.mess");
    assert_s(mess_accessibleFrom.find(
            "m.uid") != mess_accessibleFrom.end(),
            "m.mess is not accessible from m.uid");
    assert_s(mess_accessibleFrom.find(
            "u.uid") != mess_accessibleFrom.end(),
            "m.mess is not accessible from u.uid");
    assert_s(mess_accessibleFrom.find(
            "g.uid") != mess_accessibleFrom.end(),
            "m.mess is not accessible from g.uid");
    assert_s(mess_accessibleFrom.find(
            "g.gid") == mess_accessibleFrom.end(),
            "m.mess is accessible from g.gid");
    assert_s(mess_accessibleFrom.find(
            "u.uname") == mess_accessibleFrom.end(),
            "m.mess is accessible from u.uname in one link");

    std::set<string> acc_accessibleFrom = am->getGenAccessibleFrom(
            am->getGeneric("u.acc"));
    assert_s(acc_accessibleFrom.find(am->getGeneric(
            "u.uid")) != acc_accessibleFrom.end(),
            "gen acc is not accessible from gen uid");
    assert_s(acc_accessibleFrom.find(am->getGeneric(
            "g.gid")) != acc_accessibleFrom.end(),
            "gen acc is not accessible from gen gid");
    assert_s(acc_accessibleFrom.find(am->getGeneric(
            "f.fid")) == acc_accessibleFrom.end(),
            "gen acc is accessible from gen fid");

    int create = am->CreateTables();
    create += am->DeleteTables();
    create += am->CreateTables();

    assert_s(create >= 0, "create/delete/create tables failed");

    cerr << "BFS, DFS tests \n";

    Prin alice;
    alice.type = "u.uname";
    alice.value = "alice";

    list<string> bfs = am->BFS_hasAccess(alice);
    list<string>::iterator it;
    cerr << endl;
    for(it = bfs.begin(); it != bfs.end(); it++) {
        cerr << *it << ", ";
    }
    cerr << "\n" << endl;

    list<string> dfs = am->DFS_hasAccess(alice);
    assert_s(bfs.size() == dfs.size(), "bfs and dfs have different sizes");
    for(it = dfs.begin(); it != dfs.end(); it++) {
        cerr << *it << ", ";
    }
    cerr << endl;

    cerr << "=============================================" << endl;
    cerr << "raluca tests" << endl;

    Prin name_a;
    name_a.type = "users.username";
    name_a.value = "alice";
    Prin name_b;
    name_b.type = "users.username";
    name_b.value = "bob";
    Prin user1;
    user1.type = "users.userid";
    user1.value = "1";
    Prin user2;
    user2.type = "users.userid";
    user2.value = "2";
    Prin sender2;
    sender2.type = "privmsgs.senderid";
    sender2.value = "2";
    Prin rec1;
    rec1.type = "privmsgs.recid";
    rec1.value = "1";
    Prin mess9;
    mess9.type = "msgs.msgid";
    mess9.value = "9";
    Prin mess1;
    mess1.type = "msgs.msgid";
    mess1.value = "1";
    Prin text1;
    text1.type = "msgs.msgtext";
    text1.value = "hello world";

    string secretA = "secretA";
    secretA.resize(AES_KEY_BYTES);
    string secretB = "secretB";
    secretB.resize(AES_KEY_BYTES);

    assert_s(am->insertPsswd(name_a, secretA) == 0, "insert alice failed (a)");
    assert_s(am->insertPsswd(name_b, secretB) == 0, "insert bob failed (a)");

    am->insert(name_a, user1);
    am->insert(name_b, user2);
    am->insert(rec1, mess1);
    am->insert(sender2, mess1);
    am->insert(mess1, text1);

    assert_s(am->getKey(
            mess1).length() > 0, "can't access orphan key for message 1");

    cerr << "=============================================" << endl;
    cerr << "single-user tests" << endl;

    Prin u1;
    u1.type = "u.uid";
    u1.value = "1";
    Prin g5;
    g5.type = "g.gid";
    g5.value = "5";
    Prin f2;
    f2.type = "f.fid";
    f2.value = "2";

    assert_s(am->insertPsswd(alice, secretA) == 0, "insert alice failed (1)");
    am->insert(alice,u1);
    am->insert(u1,g5);
    am->insert(g5,f2);
    string alice_key = am->getKey(f2);
    LOG(test) << stringToByteInts(alice_key);
    string f2_key1 = marshallBinary(alice_key);
    assert_s(alice_key.length() > 0, "alice can't access the forum 2 key");
    am->removePsswd(alice);
    assert_s(am->getKey(
            alice).length() == 0,
            "can access alice's key with no one logged in");
    assert_s(am->getKey(
            u1).length() == 0,
            "can access user 1 key with no one logged in");
    assert_s(am->getKey(
            g5).length() == 0,
            "can access group 5 key with no one logged in");
    assert_s(am->getKey(
            f2).length() == 0,
            "can access forum 2 key with no one logged in");
    assert_s(am->insertPsswd(alice, secretA) == 0, "insert alice failed (2)");
    alice_key = am->getKey(f2);
    assert_s(
            alice_key.length() > 0,
            "forum 2 key not found when alice logs on again");
    string f2_key2 = marshallBinary(alice_key);
    assert_s(f2_key1.compare(f2_key2) == 0, "forum 2 keys are not equal");

    assert_s(am->addEquals("g.gid",
            "foo.bar") < 0,
            "should not be able to alter meta here");
    assert_s(am->addAccess("g.gid",
            "foo.bar") < 0,
            "should not be able to alter meta here");
    assert_s(am->addGives(
            "foo.bar") < 0, "should not be able to alter meta here");

    cerr << "=============================================" << endl;
    cerr << "multi-user tests" << endl;

    Prin bob;
    bob.type = "u.uname";
    bob.value = "bob";
    Prin u2;
    u2.type = "u.uid";
    u2.value = "2";
    Prin f3;
    f3.type = "f.fid";
    f3.value = "3";
    Prin a5;
    a5.type = "u.acc";
    a5.value = "5";
    Prin mlwork;
    mlwork.type = "x.mailing_list";
    mlwork.value = "work";

    assert_s(am->insertPsswd(bob, secretB) == 0, "insert bob failed (1)");
    am->insert(bob,u2);
    am->insert(u2,g5);
    assert_s(am->getKey(
            f2).length() > 0,
            "forum 2 key not accessible with both alice and bob logged on");
    am->removePsswd(alice);
    string bob_key = am->getKey(f2);
    assert_s(
            bob_key.length() > 0,"forum 2 key not accessible with bob logged on");
    string f2_key3 = marshallBinary(bob_key);
    assert_s(f2_key2.compare(
            f2_key3) == 0,
            "forum 2 key is not the same for bob as it was for alice");
    am->insert(g5,f3);
    bob_key = am->getKey(f3);
    string f3_key1 = marshallBinary(bob_key);
    assert_s(
            bob_key.length() > 0, "forum 3 key not acessible with bob logged on");
    am->removePsswd(bob);
    assert_s(am->getKey(
            alice).length() == 0,
            "can access alice's key with no one logged in");
    assert_s(am->getKey(
            bob).length() == 0,
            "can access bob's key with no one logged in");
    assert_s(am->getKey(
            u1).length() == 0,
            "can access user 1 key with no one logged in");
    assert_s(am->getKey(
            u2).length() == 0,
            "can access user 2 key with no one logged in");
    assert_s(am->getKey(
            g5).length() == 0,
            "can access group 5 key with no one logged in");
    assert_s(am->getKey(
            f2).length() == 0,
            "can access forum 2 key with no one logged in");
    assert_s(am->getKey(
            f3).length() == 0,
            "can access forum 3 key with no one logged in");
    assert_s(am->insertPsswd(alice, secretA) == 0, "insert alice failed (3)");
    alice_key = am->getKey(f3);
    assert_s(
            alice_key.length() > 0,
            "forum 3 key not accessible with alice logged on");
    string f3_key2 = marshallBinary(alice_key);
    assert_s(f3_key1.compare(
            f3_key2) == 0,
            "forum 3 key is not the same for alice as it was for bob");
    am->removePsswd(alice);
    am->insert(g5,mlwork);
    assert_s(am->getKey(
            mlwork).length() == 0,
            "can access mailing list work key with no one logged in");
    assert_s(am->insertPsswd(alice, secretA) == 0, "insert alice failed (4)");
    alice_key = am->getKey(mlwork);
    assert_s(
            alice_key.length() > 0,
            "mailing list work key inaccessible when alice is logged on");
    string mlwork_key1 = marshallBinary(alice_key);
    am->removePsswd(alice);
    assert_s(am->insertPsswd(bob, secretB) == 0, "insert bob failed (2)");
    bob_key = am->getKey(mlwork);
    assert_s(
            bob_key.length() > 0,
            "mailing list work key inaccessible when bob is logged on");
    string mlwork_key2 = marshallBinary(bob_key);
    assert_s(mlwork_key1.compare(
            mlwork_key2) == 0,
            "mailing list work key is not the same for bob as it was for alice");
    am->removePsswd(bob);

    cerr << "=============================================" << endl;
    cerr << "acylic graph, not tree tests" << endl;
    am->insert(g5,a5);
    assert_s(am->getKey(
            a5).length() == 0,
            "can access account 5 key with no one logged in");
    assert_s(am->insertPsswd(alice, secretA) == 0, "insert alice failed (5)");
    alice_key = am->getKey(a5);
    assert_s(
            alice_key.length() > 0,
            "account 5 key inaccessible when alice is logged on");
    string a5_key1 = marshallBinary(alice_key);
    am->removePsswd(alice);
    assert_s(am->insertPsswd(bob, secretB) == 0, "insert bob failed (3)");
    bob_key = am->getKey(a5);
    string a5_key2 = marshallBinary(bob_key);
    assert_s(a5_key1.compare(
            a5_key2) == 0,
            "account 5 key is not the same for bob as it was for alice");

    cerr << "=============================================" << endl;
    cerr << "orphan tests" << endl;

    am->removePsswd(bob);
    Prin m2;
    m2.type = "m.mess";
    m2.value = "2";
    Prin s6;
    s6.type = "m.sub";
    s6.value = "6";
    Prin m3;
    m3.type = "m.mess";
    m3.value = "3";
    Prin s4;
    s4.type = "m.sub";
    s4.value = "4";
    am->insert(m2, s6);
    string s6_key = am->getKey(s6);
    string m2_key = am->getKey(m2);
    string s6_key1 = marshallBinary(s6_key);
    string m2_key1 = marshallBinary(m2_key);*/
#if 0   /* XXX this is all broken because of char->string changes */
    assert_s(s6_key,
            "subject 6 key is not available after it has been inserted");
    assert_s(
            m2_key,
            "message 2 key (orphan) is not available after it has been inserted");
    am->insert(u2,m2);
    s6_key = am->getKey(s6);
    m2_key = am->getKey(m2);
    assert_s(!s6_key, "subject 6 key is available with bob not logged on");
    assert_s(!m2_key, "message 2 key is available with bob not logged on");
    assert_s(am->insertPsswd(bob, padPasswd(secretB)) == 0,
            "insert bob failed (4)");
    s6_key = am->getKey(s6);
    m2_key = am->getKey(m2);
    string s6_key2 = marshallBinary(s6_key,AES_KEY_BYTES);
    string m2_key2 = marshallBinary(m2_key,AES_KEY_BYTES);
    assert_s(s6_key, "subject 6 key is not available with bob logged on");
    assert_s(s6_key1.compare(
            s6_key2) == 0,
            "subject 6 key is not equal when orphan and for bob");
    assert_s(m2_key, "message 2 key is not available with bob logged on");
    assert_s(m2_key1.compare(
            m2_key2) == 0,
            "message 2 key is not equal when orphan and for bob");
    am->insert(m3,s4);
    unsigned char * s4_key = am->getKey(s4);
    unsigned char * m3_key = am->getKey(m3);
    string s4_key1 = marshallBinary(s4_key, AES_KEY_BYTES);
    string m3_key1 = marshallBinary(m3_key, AES_KEY_BYTES);
    assert_s(s4_key,
            "subject 4 key is not available after it has been inserted");
    assert_s(
            m3_key,
            "message 3 key (orphan) is not available after it has been inserted");
    am->insert(u2,m3);
    s4_key = am->getKey(s4);
    m3_key = am->getKey(m3);
    string s4_key2 = marshallBinary(s4_key, AES_KEY_BYTES);
    string m3_key2 = marshallBinary(m3_key, AES_KEY_BYTES);
    assert_s(s4_key, "subject 4 key is not available with bob logged on");
    assert_s(s4_key1.compare(
            s4_key2) == 0,
            "subject 4 key is not equal when orphan and for bob");
    assert_s(m3_key,
            "message 3 key (orphan) is not available with bob logged on");
    assert_s(m3_key1.compare(
            m3_key2) == 0,
            "message 3 key is not equal when orphan and for bob");
    am->removePsswd(bob);
    s4_key = am->getKey(s4);
    m3_key = am->getKey(m3);
    assert_s(!s4_key,
            "subject 4 key is not available after it has been inserted");
    assert_s(
            !m3_key,
            "message 3 key (orphan) is not available after it has been inserted");
    assert_s(am->insertPsswd(bob, padPasswd(secretB)) == 0,
            "insert bob failed (5)");
    s4_key = am->getKey(s4);
    m3_key = am->getKey(m3);
    string s4_key3 = marshallBinary(s4_key, AES_KEY_BYTES);
    string m3_key3 = marshallBinary(m3_key, AES_KEY_BYTES);
    assert_s(s4_key, "subject 4 key is not available with bob logged on");
    assert_s(s4_key1.compare(
            s4_key3) == 0,
            "subject 4 key is not equal when orphan and for bob (take 2)");
    assert_s(m3_key,
            "message 3 key (orphan) is not available with bob logged on");
    assert_s(m3_key1.compare(
            m3_key3) == 0,
            "message 3 key is not equal when orphan and for bob (take 2)");

    Prin m4;
    m4.type = "m.mess";
    m4.value = "4";
    Prin m5;
    m5.type = "m.mess";
    m5.value = "5";
    Prin s5;
    s5.type = "m.sub";
    s5.value = "5";
    Prin s7;
    s7.type = "m.sub";
    s7.value = "7";
    am->insert(m4,s6);
    unsigned char * m4_key = am->getKey(m4);
    s6_key = am->getKey(s6);
    string m4_key1 = marshallBinary(m4_key, AES_KEY_BYTES);
    string s6_key3 = marshallBinary(s6_key, AES_KEY_BYTES);
    assert_s(m4_key, "orphan message 4 key not available");
    assert_s(
            s6_key,
            "subject 6 key not available when orphan message 4 key available and bob logged on");
    assert_s(s6_key1.compare(s6_key3) == 0, "subject 6 key not the same");
    am->removePsswd(bob);
    m4_key = am->getKey(m4);
    s6_key = am->getKey(s6);
    string m4_key2 = marshallBinary(m4_key, AES_KEY_BYTES);
    string s6_key4 = marshallBinary(s6_key, AES_KEY_BYTES);
    assert_s(m4_key, "orphan message 4 key not available after bob logs off");
    assert_s(m4_key1.compare(
            m4_key2) == 0,
            "orphan message 4 key is different after bob logs off");
    assert_s(
            s6_key,
            "subject 6 key not available when orphan message 4 key available");
    assert_s(s6_key1.compare(
            s6_key3) == 0,
            "subject 6 key not the same after bob logs off");

    am->insert(m5,s5);
    am->insert(m5,s7);
    unsigned char * m5_key = am->getKey(m5);
    unsigned char * s5_key = am->getKey(s5);
    unsigned char * s7_key = am->getKey(s7);
    string m5_key1 = marshallBinary(m5_key, AES_KEY_BYTES);
    string s5_key1 = marshallBinary(s5_key, AES_KEY_BYTES);
    string s7_key1 = marshallBinary(s7_key, AES_KEY_BYTES);
    assert_s(m5_key, "message 5 key (orphan) not available");
    assert_s(s5_key, "subject 5 key (orphan) not available");
    assert_s(s7_key, "subject 7 key (orphan) not available");
    am->insert(u1,m5);
    m5_key = am->getKey(m5);
    s5_key = am->getKey(s5);
    s7_key = am->getKey(s7);
    assert_s(!am->getKey(alice), "alice is not logged off");
    assert_s(!m5_key, "message 5 key available with alice not logged on");
    assert_s(!s5_key, "subject 5 key available with alice not logged on");
    assert_s(!s7_key, "subject 7 key available with alice not logged on");
    assert_s(am->insertPsswd(alice, padPasswd(secretA)) == 0,
            "insert alice failed (6)");
    m5_key = am->getKey(m5);
    s5_key = am->getKey(s5);
    s7_key = am->getKey(s7);
    string m5_key2 = marshallBinary(m5_key, AES_KEY_BYTES);
    string s5_key2 = marshallBinary(s5_key, AES_KEY_BYTES);
    string s7_key2 = marshallBinary(s7_key, AES_KEY_BYTES);
    assert_s(m5_key, "message 5 key not available with alice logged on");
    assert_s(m5_key1.compare(m5_key2) == 0, "message 5 key is different");
    assert_s(s5_key, "subject 5 key not available with alice logged on");
    assert_s(s5_key1.compare(s5_key2) == 0, "subject 5 key is different");
    assert_s(s7_key, "subject 7 key not available with alice logged on");
    assert_s(s7_key1.compare(s7_key2) == 0, "subject 7 key is different");

    Prin chris;
    chris.type = "u.uname";
    chris.value = "chris";
    Prin u3;
    u3.type = "u.uid";
    u3.value = "3";
    Prin m15;
    m15.type = "m.mess";
    m15.value = "15";
    Prin s24;
    s24.type = "m.sub";
    s24.value = "24";
    am->insert(u3, m15);
    unsigned char * m15_key = am->getKey(m15);
    assert_s(m15_key, "cannot access message 15 key (orphan)");
    string m15_key1 = marshallBinary(m15_key, AES_KEY_BYTES);
    unsigned char * u3_key = am->getKey(u3);
    assert_s(u3_key, "cannot access user 3 key (orphan)");
    string u3_key1 = marshallBinary(u3_key, AES_KEY_BYTES);
    am->insert(m15, s24);
    unsigned char * s24_key = am->getKey(s24);
    assert_s(s24_key, "cannot access subject 24 key (orphan)");
    string s24_key1 = marshallBinary(s24_key, AES_KEY_BYTES);
    assert_s(am->insertPsswd(chris, padPasswd(secretC)) == 0,
            "insert chris failed (1)");
    unsigned char * chris_key = am->getKey(chris);
    assert_s(chris_key, "cannot access chris key with chris logged on");
    string chris_key1 = marshallBinary(chris_key, AES_KEY_BYTES);
    am->insert(chris, u3);
    chris_key = am->getKey(chris);
    assert_s(chris_key, "cannot access chris key after chris->u3 insert");
    string chris_key2 = marshallBinary(chris_key, AES_KEY_BYTES);
    assert_s(chris_key1.compare(
            chris_key2) == 0,
            "chris key is different for orphan and chris logged on");

    am->removePsswd(chris);
    assert_s(!am->getKey(chris), "can access chris key with chris offline");
    assert_s(!am->getKey(u3), "can access user 3 key with chris offline");
    assert_s(!am->getKey(m15), "can access message 15 key with chris offline");
    assert_s(!am->getKey(s24), "can access subject 24 key with chris offline");

    assert_s(am->insertPsswd(chris, padPasswd(secretC)) == 0,
            "insert chris failed (2)");
    chris_key = am->getKey(chris);
    assert_s(chris_key,
            "cannot access chris key with chris logged on after logging off");
    string chris_key3 = marshallBinary(chris_key, AES_KEY_BYTES);
    assert_s(chris_key1.compare(
            chris_key3) == 0,
            "chris key is different for orphan and chris logged on after logging off");
    u3_key = am->getKey(u3);
    assert_s(
            u3_key,
            "cannot access user 3 key with chris logged on after logging off");
    string u3_key2 = marshallBinary(u3_key, AES_KEY_BYTES);
    assert_s(u3_key1.compare(
            u3_key2) == 0,
            "user 3 key is different for orphan and chris logged on after logging off");
    m15_key = am->getKey(m15);
    assert_s(
            m15_key,
            "cannot access message 15 key with chris logged on after logging off");
    string m15_key2 = marshallBinary(m15_key, AES_KEY_BYTES);
    assert_s(m15_key1.compare(
            m15_key2) == 0,
            "message 15 key is different for orphan and chris logged on after logging off");
    s24_key = am->getKey(s24);
    assert_s(
            s24_key,
            "cannot access subject 24 key with chris logged on after logging off");
    string s24_key2 = marshallBinary(s24_key, AES_KEY_BYTES);
    assert_s(s24_key1.compare(
            s24_key2) == 0,
            "subject 24 key is different for orphan and chris logged on after logging off");

    Prin s16;
    s16.type = "m.sub";
    s16.value = "16";
    unsigned char * s16_key = am->getKey(s16);
    string s16_key1 = marshallBinary(s16_key, AES_KEY_BYTES);
    assert_s(s16_key, "orphan subject 16 did not get a key generated for it");
    am->insert(m15, s16);
    s16_key = am->getKey(s16);
    string s16_key2 = marshallBinary(s16_key, AES_KEY_BYTES);
    assert_s(s16_key, "subject 16 does not have key being de-orphanized");
    assert_s(s16_key1.compare(
            s16_key2) == 0,
            "subject 16 has a different key after being orphanized");
    am->removePsswd(chris);
    assert_s(!am->getKey(s16), "can access subject 16 key with chris offline");
    assert_s(am->insertPsswd(chris, padPasswd(secretC)) == 0,
            "insert chris failed (2)");
    s16_key = am->getKey(s16);
    string s16_key3 = marshallBinary(s16_key, AES_KEY_BYTES);
    assert_s(s16_key,
            "subject 16 does not have key after chris logs off and on");
    assert_s(s16_key1.compare(
            s16_key2) == 0,
            "subject 16 has a different key after chris logs out and back in");

    cerr << "=============================================" << endl;
    cerr << "remove tests" << endl;
    assert_s(am->insertPsswd(bob, padPasswd(secretB)) == 0,
            "insert bob failed (6)");
    s4_key = am->getKey(s4);
    assert_s(s4_key, "cannot access subject 4 key with bob logged on");
    string s4_key4 = marshallBinary(s4_key, AES_KEY_BYTES);
    assert_s(s4_key1.compare(s4_key4) == 0, "Subject 4 has changed");
    s6_key = am->getKey(s6);
    assert_s(s6_key, "cannot access subject 6 key with bob logged on");
    string s6_key5 = marshallBinary(s6_key, AES_KEY_BYTES);
    assert_s(s6_key1.compare(s6_key5) == 0, "Subject 6 has changed");
    m2_key = am->getKey(m2);
    assert_s(m2_key, "cannot access message 2 key with bob logged on");
    string m2_key3 = marshallBinary(m2_key, AES_KEY_BYTES);
    assert_s(m2_key1.compare(m2_key3) == 0, "Message 2 has changed");
    m3_key = am->getKey(m3);
    assert_s(m3_key, "cannot access message 3 key with bob logged on");
    string m3_key4 = marshallBinary(m3_key, AES_KEY_BYTES);
    assert_s(m3_key1.compare(m3_key4) == 0, "Message 3 has changed");
    unsigned char * u2_key = am->getKey(u2);
    assert_s(u2_key, "cannot access user 2 key with bob logged on");
    string u2_key1 = marshallBinary(u2_key, AES_KEY_BYTES);

    am->removePsswd(alice);
    unsigned char * g5_key = am->getKey(g5);
    assert_s(g5_key, "cannot access group 5 key with bob logged on");
    string g5_key1 = marshallBinary(g5_key, AES_KEY_BYTES);
    unsigned char * f2_key = am->getKey(f2);
    assert_s(f2_key, "cannot access forum 2 key with bob logged on");
    string f2_key4 = marshallBinary(f2_key, AES_KEY_BYTES);
    assert_s(f2_key1.compare(f2_key4) == 0, "Forum 2 key has changed");
    unsigned char * f3_key = am->getKey(f3);
    assert_s(f3_key, "cannot access forum 3 key with bob logged on");
    string f3_key3 = marshallBinary(f3_key, AES_KEY_BYTES);
    assert_s(f3_key1.compare(f3_key3) == 0, "Forum 3 key has changed");
    unsigned char * a5_key = am->getKey(a5);
    assert_s(a5_key, "cannot access account 5 key with bob logged on");
    string a5_key3 = marshallBinary(a5_key, AES_KEY_BYTES);
    assert_s(a5_key1.compare(a5_key3) == 0, "Account 5 key has changed");
    unsigned char * u1_key = am->getKey(u1);
    assert_s(!u1_key, "user 1 key available when Alice not logged on");
    assert_s(am->insertPsswd(alice, padPasswd(secretA)) == 0,
            "insert alice failed (6)");

    am->remove(u2, g5);
    s4_key = am->getKey(s4);
    assert_s(s4_key, "cannot access subject 4 key with bob logged on");
    string s4_key5 = marshallBinary(s4_key, AES_KEY_BYTES);
    assert_s(s4_key1.compare(s4_key5) == 0, "Subject 4 has changed");
    s6_key = am->getKey(s6);
    assert_s(s6_key, "cannot access subject 6 key with bob logged on");
    string s6_key6 = marshallBinary(s6_key, AES_KEY_BYTES);
    assert_s(s6_key1.compare(s6_key6) == 0, "Subject 6 has changed");
    m2_key = am->getKey(m2);
    assert_s(m2_key, "cannot access message 2 key with bob logged on");
    string m2_key4 = marshallBinary(m2_key, AES_KEY_BYTES);
    assert_s(m2_key1.compare(m2_key4) == 0, "Message 2 has changed");
    m3_key = am->getKey(m3);
    assert_s(m3_key, "cannot access message 3 key with bob logged on");
    string m3_key5 = marshallBinary(m3_key, AES_KEY_BYTES);
    assert_s(m3_key1.compare(m3_key5) == 0, "Message 3 has changed");
    g5_key = am->getKey(g5);
    assert_s(g5_key, "cannot access group 5 key with alice logged on");
    string g5_key2 = marshallBinary(g5_key, AES_KEY_BYTES);
    assert_s(g5_key1.compare(g5_key2) == 0, "Group 5 key has changed");
    f2_key = am->getKey(f2);
    assert_s(f2_key, "cannot access forum 2 key with alice logged on");
    string f2_key5 = marshallBinary(f2_key, AES_KEY_BYTES);
    assert_s(f2_key1.compare(f2_key5) == 0, "Forum 2 key has changed");
    f3_key = am->getKey(f3);
    assert_s(f3_key, "cannot access forum 3 key with alice logged on");
    string f3_key4 = marshallBinary(f3_key, AES_KEY_BYTES);
    assert_s(f3_key1.compare(f3_key4) == 0, "Forum 3 key has changed");
    a5_key = am->getKey(a5);
    assert_s(a5_key, "cannot access account 5 key with alice logged on");
    string a5_key4 = marshallBinary(a5_key, AES_KEY_BYTES);
    assert_s(a5_key1.compare(a5_key4) == 0, "Account 5 key has changed");

    am->removePsswd(alice);
    g5_key = am->getKey(g5);
    assert_s(!g5_key, "group 5 key available when alice is logged off");
    a5_key = am->getKey(a5);
    assert_s(!a5_key, "account 5 key available when alice is logged off");
    f2_key = am->getKey(f2);
    assert_s(!f2_key, "forum 2 key available when alice is logged off");
    f3_key = am->getKey(f3);
    assert_s(!f3_key, "forum 3 key available when alice is logged off");

    assert_s(am->insertPsswd(chris, padPasswd(secretC)) == 0,
            "insert chris failed (3)");
    s24_key = am->getKey(s24);
    assert_s(s24_key, "subject 24 key is not accessible with chris logged on");
    string s24_key3 = marshallBinary(s24_key, AES_KEY_BYTES);
    assert_s(s24_key1.compare(
            s24_key3) == 0, "subject 24 key is not the same");
    m15_key = am->getKey(m15);
    assert_s(m15_key, "message 15 key is not accessible with chris logged on");
    string m15_key3 = marshallBinary(m15_key, AES_KEY_BYTES);
    assert_s(m15_key1.compare(
            m15_key3) == 0, "message 15 key is not the same");
    u3_key = am->getKey(u3);
    assert_s(u3_key, "user 3 key is not accessible with chris logged on");
    string u3_key3 = marshallBinary(u3_key, AES_KEY_BYTES);
    assert_s(u3_key1.compare(u3_key3) == 0, "user 3 key is not the same");

    am->remove(u3,m15);
    s24_key = am->getKey(s24);
    assert_s(!s24_key, "subject 24 key is accessible after removal");
    m15_key = am->getKey(m15);
    assert_s(!m15_key, "message 15 key is accessible after removal");
    u3_key = am->getKey(u3);
    assert_s(u3_key,
            "user 3 key is not accessible with chris after u3->m15 removal");
    string u3_key4 = marshallBinary(u3_key, AES_KEY_BYTES);
    assert_s(u3_key1.compare(
            u3_key4) == 0,
            "user 3 key is not the same after u3->m15 removal");

    am->remove(g5,f3);
    assert_s(am->insertPsswd(alice, padPasswd(secretA)) == 0,
            "insert alice failed (7)");
    g5_key = am->getKey(g5);
    assert_s(g5_key, "cannot access group 5 key with alice logged on");
    string g5_key3 = marshallBinary(g5_key, AES_KEY_BYTES);
    assert_s(g5_key1.compare(g5_key3) == 0, "Group 5 key has changed");
    f2_key = am->getKey(f2);
    assert_s(f2_key, "cannot access forum 2 key with alice logged on");
    string f2_key6 = marshallBinary(f2_key, AES_KEY_BYTES);
    assert_s(f2_key1.compare(f2_key6) == 0, "Forum 2 key has changed");
    a5_key = am->getKey(a5);
    assert_s(a5_key, "cannot access account 5 key with alice logged on");
    string a5_key5 = marshallBinary(a5_key, AES_KEY_BYTES);
    assert_s(a5_key1.compare(a5_key5) == 0, "Account 5 key has changed");
    g5_key = am->getKey(g5);
    f3_key = am->getKey(f3);
    assert_s(!f3_key, "forum 3 key available when alice is logged off");

    am->removePsswd(bob);
    s6_key = am->getKey(s6);
    assert_s(s6_key, "subject 6 key, attached to orphan m4 not accessible");
    string s6_key7 = marshallBinary(s6_key, AES_KEY_BYTES);
    assert_s(s6_key1.compare(s6_key7) == 0, "subject 6 key has changed");
    m4_key = am->getKey(m4);
    assert_s(m4_key, "message 4 key (orpahn) not available");
    string m4_key3 = marshallBinary(m4_key, AES_KEY_BYTES);
    assert_s(m4_key1.compare(m4_key3) == 0, "message 4 key has changed");

    am->remove(m4,s6);
    m3_key = am->getKey(m3);
    assert_s(!m3_key, "message 3 key available when bob is logged off");
    m2_key = am->getKey(m2);
    assert_s(!m2_key, "message 2 key available when bob is logged off");
    s6_key = am->getKey(s6);
    assert_s(!s6_key, "subject 6 key available when bob is logged off");
    m4_key = am->getKey(m4);
    assert_s(m4_key, "message 4 key (orpahn) not available after remove");
    string m4_key4 = marshallBinary(m4_key, AES_KEY_BYTES);
    assert_s(m4_key1.compare(
            m4_key4) == 0, "message 4 key has changed after remove");

    cerr << "=============================================" << endl;
    cerr << "threshold tests" << endl;

    am->removePsswd(alice);
    string g50_key1;
    unsigned char * g50_key;
    for (unsigned int i = 6; i < 110; i++) {
        Prin group;
        group.type = "g.gid";
        group.value = strFromVal(i);
        am->insert(u3,group);
        if(i == 50) {
            g50_key = am->getKey(group);
            assert_s(g50_key,
                    "could not access g50 key just after it's inserted");
            g50_key1 = marshallBinary(g50_key, AES_KEY_BYTES);
        }
    }

    am->removePsswd(chris);
    Prin g50;
    g50.type = "g.gid";
    g50.value = "50";
    g50_key = am->getKey(g50);
    assert_s(!g50_key, "g50 key available after chris logs off");
    assert_s(am->insertPsswd(chris, padPasswd(secretC)) == 0,
            "insert chris failed (4)");
    PrinKey g50_pkey = am->getUncached(g50);
    assert_s(g50_pkey.len != 0,
            "can't access g50 key after chris logs back on");
    g50_key = am->getKey(g50);
    string g50_key2 = marshallBinary(g50_key,AES_KEY_BYTES);
    assert_s(g50_key1.compare(
            g50_key2) == 0,
            "group 50 key is different after chris logs on and off");

    for (unsigned int i = 6; i < 110; i++) {
        Prin group;
        group.type = "g.gid";
        group.value = strFromVal(i);
        am->remove(u3,group);
    }

    g50_key = am->getKey(g50);
    assert_s(!g50_key,
            "g50 key exists after the hundred group keys have been removed");

    delete am;

    /* AccessManager * am;

       // This test is no longer valid due to orphans.

       am = new AccessManager(new Connect(tc.host, tc.user, tc.pass, tc.db));

       am->addEquals("i.uid","u.uid");
       am->givesPsswd("u.uname");
       am->addAccessTo("u.uid", "u.uname");

       //alice, bob join
       am->insertPsswd("alice", padPasswd(secretA));
       am->insertPsswd("bob", padPasswd(secretB));

       assert_s(am->getKey("i.uid", "1") == NULL, "the key is not NULL");

       cerr << "passed i.uid test \n";

       am->finish();

       am = new AccessManager(new Connect(tc.host, tc.user, tc.pass, tc.db));

       //OUR EXAMPLE TEST
     * */

    /* cerr << "=====================================\n";

       cout << "our example test" << endl;
       am->addAccessTo("g.gid","g.uid");

       am->addEquals("g.uid","u.uid");
       am->givesPsswd("u.uname");
       am->addAccessTo("u.uid", "u.uname");


       //alice, bob, chris join
       //unsigned char * palice = padPasswd(111);
       am->insertPsswd("alice", padPasswd(111));
       am->insertPsswd("alice", padPasswd(111));
       am->insertPsswd("bob", padPasswd(222));
       am->insertPsswd("chris", padPasswd(333));

       am->insert(makeList("u.uid", "u.uname"), makeList("1","alice"));
       am->insert(makeList("u.uid", "u.uname"), makeList("2","bob"));
       am->insert(makeList("u.uid", "u.uname"), makeList("3","chris"));

       //alice is in group 11, bob is in group 22, chris in 11
       am->insert(makeList("g.gid", "g.uid"), makeList("11", "1"));
       am->insert(makeList("g.gid", "g.uid"), makeList("22", "2"));
       am->insert(makeList("g.gid", "g.uid"), makeList("11", "3"));


       //alice leaves
       cerr << "d\n";
       am->deletePsswd("alice");

       cerr << "e\n";
       unsigned char * keychris = am->getKey("g.gid", "11");

       cerr << "f\n";
       //alice comes back, chris leaves

       am->insertPsswd("alice", padPasswd(111));
       cerr << "g\n";
       am->deletePsswd("chris");

       unsigned char * keyalice = am->getKey("g.gid", "11");
       cerr << (keyalice == NULL) << " where true is " << true << endl;
       cerr << "h\n";
       assert_s(isEqual(keyalice, keychris, AES_KEY_BYTES), "alice/chris gid
          keys different");


       cerr << "j\n";
       //alice leaves as well
       am->deletePsswd("alice");
       assert_s(am->getKey("g.gid", "11") == NULL, "gid key should not be
          available when no user online");
       cerr << "i\n";
       assert_s(am->getKey("g.gid", "22") != NULL, "bob is online but key is
          null!");

       am->deletePsswd("bob");
       cerr << "j\n";

       assert_s(am->getKey("g.gid","22") == NULL, "bob left yet his gid key is
          still available");
       cerr << "k\n";



       am->finish();

       cerr << "our example OK\n";

       cerr << "LONG STRINGS OF EQUALITY TESTING!"  << endl;

       am = new AccessManager(new Connect(tc.host, tc.user, tc.pass, tc.db));

       am->givesPsswd("u.uname");
       am->addEquals("u.uid","p.uid");
       am->addEquals("p.uid","t.uid");
       am->addEquals("t.uid","c.uid");
       am->addEquals("c.uid","u.uid");
       am->addAccessTo("u.uid","u.uname");
       am->insertPsswd("alice", padPasswd(111));
       am->insert(makeList("u.uid","u.uname"), makeList("1","alice"));
       unsigned char * uukey = am->getKey("u.uid","1");

       assert_s(am->getKey("u.uid","1") != NULL,"u.uid should have key");
       assert_s(am->getKey("p.uid","1") != NULL,"p.uid should have key");
       assert_s(isEqual(am->getKey("p.uid","1"),uukey,AES_KEY_BYTES),"key not
          equal");
       assert_s(am->getKey("t.uid","1") != NULL,"t.uid should have key");
       assert_s(isEqual(am->getKey("t.uid","1"),uukey,AES_KEY_BYTES),"key not
          equal");
       assert_s(am->getKey("c.uid","1") != NULL,"c.uid should have key");
       assert_s(isEqual(am->getKey("c.uid","1"),uukey,AES_KEY_BYTES),"key not
          equal");

       am->finish();


       cerr << "Test\n";
       am = new AccessManager(new Connect(tc.host, tc.user, tc.pass, tc.db));

       am->addEquals("t1.id", "users.id");
       am->givesPsswd("users.username");
       am->addAccessTo("users.id", "users.username");
       am->insertPsswd("alice", padPasswd(111));
       am->deletePsswd("alice");
       am->insertPsswd("alice", padPasswd(111));
       am->insert(makeList("users.id", "users.username"), makeList("1",
          "alice"));
       assert_s(am->getKey("users.id", "1") != NULL, "access manager should
          have key");
       assert_s(am->getKey("t1.id", "1") != NULL, "access manager should have
          key");
       LOG(test) << myPrint(am->getKey("users.id", "1"), AES_KEY_BYTES);

       am->finish();

       cerr << "\n";

       cerr << "TEST: users properly log out \n";

       am = new AccessManager(new Connect(tc.host, tc.user, tc.pass, tc.db));

       cerr << "1-----------------------------------------------" << endl;
       assert_s(0==am->addEquals("users.id", "t1.id"), "operation failed");
       assert_s(0==am->givesPsswd("users.username"), "operation failed");
       assert_s(0==am->addAccessTo("users.id", "users.username"), "operation
          failed");
       assert_s(0==am->addAccessTo("usergroup.gid", "usergroup.uid"),
          "operation failed");
       assert_s(0==am->addEquals("usergroup.gid", "g.gid"), "operation
          failed");
       assert_s(0==am->addEquals("usergroup.uid","users.id"), "operation
          failed");
       assert_s(0==am->insertPsswd("alice", padPasswd(111)),
          "operation failed");
       assert_s(0==am->insertPsswd("bob", padPasswd(222)),
          "operation failed");
       assert_s(0==am->insert(makeList("users.id", "users.username"),
          makeList("1", "alice")), "operation failed");
       assert_s(0==am->insert(makeList("users.id", "users.username"),
          makeList("2", "bob")), "operation failed");

       assert_s(0==am->insert(makeList("usergroup.gid", "usergroup.uid"),
          makeList("1", "1")), "operation failed");
       assert_s(0==am->insert(makeList("usergroup.gid", "usergroup.uid"),
          makeList("1", "2")), "operation failed");
       cerr << "2-----------------------------------------------" << endl;
       unsigned char * key = am->getKey("usergroup.gid", "1");
       assert_s(key != NULL, "access manager should have key");

       assert_s(am->getKey("usergroup.uid", "1") != NULL, "user is online");
       assert_s(am->getKey("usergroup.uid", "2") != NULL, "user is online");

       am->deletePsswd("bob");

       unsigned char * key2 = am->getKey("usergroup.gid", "1");
       assert_s(key2 != NULL, "access manager should have key");
       assert_s(isEqual(key, key2, AES_KEY_BYTES), "keys should be equal");

       am->deletePsswd("alice");

       assert_s(am->getKey("usergroup.gid", "1") == NULL, "no one should have
          access now");
       assert_s(am->getKey("usergroup.uid", "1") == NULL, "no one should have
          access now");
       assert_s(am->getKey("usergroup.uid", "2") == NULL, "no one should have
          access now");

       //assert_s(am->hasAccessTo("t1.id").length() > 0, "at least
          users.username should have access to it");

       //assert_s(am->hasAccessTo("g.gid").length() > 0, "at least
          usergroup.uid should have access to it");

       assert_s(am->insert(makeList("users.id", "users.username"),
          makeList("3", "anonymous")) < 0, "operation failed");

       am->finish();

       cerr << "TESTS OK \n";


       cerr <<" ===========================\n TEST ORPHAN \n";

       am = new AccessManager(new Connect(tc.host, tc.user, tc.pass, tc.db));

       assert_s(0<=am->givesPsswd("u.uname"), "problem with gives psswd");

       assert_s(0<=am->addAccessTo("u.uid","u.uname"), "problem with
          addaccces");
       //sender and reciever of message mid
       assert_s(0<=am->addAccessTo("m.mid","m.sid"), "problem with
          addaccces");
       assert_s(0<=am->addAccessTo("m.mid","m.rid"), "problem with
          addaccces");

       assert_s(0<=am->addEquals("m.sid","u.uid"), "problem with addequals");
       assert_s(0<=am->addEquals("u.uid","m.rid"), "problem with addequals");


       unsigned char * key1 = am->getKey("m.mid", "1");

       assert_s(key1 != NULL, "should get an orphan principal's key");

       assert_s(0<=am->insertPsswd("alice", padPasswd(111)), "insert pss
          failed");

       assert_s(0<=am->insert(makeList("u.uid", "u.uname"), makeList("1",
          "alice")), "insert faield");

       assert_s(0<=am->insert(makeList("m.mid", "m.rid"), makeList("1", "1")),
          "insert failed");

       unsigned char * key11 = am->getKey("m.mid", "1");

       assert_s(isEqual(key11, key1, AES_KEY_BYTES), "keys should be equal");

       assert_s(0<=am->deletePsswd("alice"), "delete psswd failed");

       assert_s(am->getKey("m.mid", "1") == NULL, "should not be orphaned any
          more");


       cerr << "orphan test OK\n";

       am->finish();

       //return;


       am = new AccessManager(new Connect(tc.host, tc.user, tc.pass, tc.db));

       cerr << "remove test \n";

       am->addAccessTo("g.gid","g.uid");
       am->addEquals("g.uid","u.uid");
       am->givesPsswd("u.uname");
       am->addAccessTo("u.uid", "u.uname");

       //alice, bob, chris join
       //unsigned char * palice = padPasswd(111);
       am->insertPsswd("alice", padPasswd(111));
       am->insertPsswd("bob", padPasswd(222));
       am->insertPsswd("chris", padPasswd(333));

       am->insert(makeList("u.uid", "u.uname"), makeList("1","alice"));
       am->insert(makeList("u.uid", "u.uname"), makeList("2","bob"));
       am->insert(makeList("u.uid", "u.uname"), makeList("3","chris"));

       //alice is in group 11, bob is in group 22, chris in 11
       am->insert(makeList("g.gid", "g.uid"), makeList("11", "1"));
       am->insert(makeList("g.gid", "g.uid"), makeList("22", "2"));
       am->insert(makeList("g.gid", "g.uid"), makeList("11", "3"));

       //am->insertPsswd("alice", padPasswd(111));

       assert_s(am->getKey("g.gid", "11") != NULL, "gid key should be
          available for Alice");

       //remove Alice's permission
       am->remove(makeList("g.gid", "g.uid"), makeList("11","1"));

       //I think this should be like so
       assert_s(am->getKey("g.gid", "11") != NULL, "Chris should still have
          permission");

       am->insert(makeList("g.gid", "g.uid"), makeList("11", "1"));

       assert_s(am->getKey("g.gid", "11") != NULL, "gid key should be
          available for Alice");

       //remove Alice's permission and log Chris off
       am->remove(makeList("g.gid", "g.uid"), makeList("11","1"));
       am->deletePsswd("chris");

       assert_s(am->getKey("g.gid", "11") == NULL, "gid key not should be
          available for Alice");

       am->finish();
     */

#endif  /* XXX end of broken */

}

typedef struct Stats {
    int total_queries;
    int queries_failed;
    double mspassed;
    int worker;
    Stats() {
        total_queries = 0;
        queries_failed = 0;
        mspassed = 0.0;
        worker = 0;
    }
} Stats;

//static Stats * myStats;

/*
 * - if execQuery is true, the query is sent to the DB
 * - if encryptQuery is true, the query is being rewritten
 * - if conn != NULL, use this connection to send queries to the DBMS
 * - if outputfile != "", log encrypted query here
 * - if !allowExecFailures, we stop on failed queries sent to the DBMS
 * - logFreq indicates frequency of logging
 * - if execEncQuery is true the query is execute via cryptdb (query encrypted and results decrypted), the previous flags
 * are not taken into account any more
 */
/*
static void
runQueriesFromFile(EDBProxy * cl, string queryFile, bool execQuery, bool encryptQuery, Connect * conn,
        string outputfile, bool allowExecFailures,  bool execEncQuery, Stats * stats, int logFreq)
throw (CryptDBError)
{
    cerr << "runQueriesFromFile uses old EDBProxy -- not run" << endl;
    ifstream infile(queryFile);
    ofstream * outfile = NULL;

    assert_s(infile.is_open(), "cannot open file " + queryFile);
    assert_s(!execQuery || conn, "conn is null, while execQuery is true");

    bool outputtranslation = false;
    if (outputfile != "") {
        outfile = new ofstream(outputfile);
        outputtranslation = true;
    }

    string query;
    list<string> queries;

    Timer t;
    while (!infile.eof()) {
        query = getQuery(infile);

        if (query.length() == 0) {
            continue;
        }

        if (!execEncQuery) {
            if (encryptQuery) {
                try{
                    bool temp;
                    queries = cl->rewriteEncryptQuery(query+";", temp);
                } catch(...) {
                    cerr << "query " << query << "failed";
                    cerr << "worker exits\n";
                    exit(-1);
                }
            } else {
                queries = list<string>(1, query);
            }
            stats->total_queries++;
            if (stats->total_queries % logFreq == 0) {
                cerr << queryFile << ": " << stats->total_queries << " so far\n";
            }
            for (list<string>::iterator it = queries.begin(); it != queries.end(); it++) {
                if  (outputtranslation) {
                    *outfile << *it << "\n";
                }
                if (execQuery) {
                    bool outcome = conn->execute(*it);
                    if (allowExecFailures) {
                        if (!outcome) {
                            stats->queries_failed++;
                        }
                    } else {
                        assert_s(outcome, "failed to execute query " + *it);
                    }
                }
            }
        } else {
            ResType r = cl->execute(query);
            assert_s(r.ok, "query failed");
            //cerr << "results returned " << r.rows.size() << "\n";
            stats->total_queries++;
        }
    }

    stats->mspassed = t.lap_ms();

    infile.close();
    if (outputtranslation) {
        outfile->close();
    }

}
*/

/*static void
dotrain(EDBProxy * cl, string createsfile, string querypatterns, string exec) {
    cl->execute(string("train ") + " 1 " + createsfile + " " + querypatterns + " " + exec);
}

static string * workloads;
static string resultFile;

static string
fixedRepr(int i) {
    string s = StringFromVal(i);
    while (s.length() < 5) {
        s = "0" + s;
    }
    return s;
}


static void
assignWork(string queryfile, int noWorkers,   int totalLines, int noRepeats, bool split) {
    int blah = system("mkdir pieces");
    assert_s(system("rm -f pieces") >= 0, "problem when removing pieces");
    if (false) {
        LOG(test) << blah;
    }

    ifstream infile(queryfile);

    workloads = new string[noWorkers];

    int linesPerWorker = totalLines / noWorkers;
    int linesPerLastWorker = totalLines - (noWorkers-1)*linesPerWorker;

    string query;

    if (!infile.is_open()) {
        cerr << "cannot open " + queryfile << "\n";
        exit(1);
    }

    //prepare files
    for (int i = 0; i < noWorkers; i++) {
        string workload;
        if (!split) {

            workload = queryfile + fixedRepr(i);

        } else {

            workload = string("pieces/piece") + fixedRepr(i);

            ofstream outfile(workload);

            cerr << "creating worker workload " << workload << "\n";
            if (!outfile.is_open()) {
                cerr << "cannot open file " << workload << "\n";
                infile.close();
                exit(1);
            }

            int lines = 0;
            if (i == noWorkers-1) {
                lines = linesPerLastWorker;
            } else {
                lines = linesPerWorker;
            }
            for (int j = 0; j < lines; j++) {
                getline(infile, query);
                if (query != "") {
                    outfile << query << "\n";
                }
            }
            outfile.close();
        }

        workloads[i] = workload;

        //we need to concatenate the outfile with itself noRepeats
        if (noRepeats > 1) {
            assert_s(system("touch temp;") >= 0, "problem when creating temp");
            for (int j = 0; j < noRepeats; j++) {
                assert_s(system((string("cat temp ") + workload + " > temp2;").c_str()) >= 0,  "problem when cat");
                assert_s(system("mv temp2 temp") >= 0, "problem when moving");
            }
            assert_s(system(("mv temp " + workload).c_str()) >= 0, "problem when moving");
        }
    }

    infile.close();
    }

static void __attribute__((noreturn))
workerFinish() {

    ofstream resFile(resultFile+StringFromVal(myStats->worker));
    assert_s(resFile.is_open(), " could not open file " + resultFile);

    resFile << myStats->worker <<  " " << myStats->queries_failed << " " << myStats->total_queries << " "
                << myStats->mspassed << "\n";
    resFile.close();
    string output =  "worker " + StringFromVal(myStats->worker) + "  queriesfailed " + StringFromVal(myStats->queries_failed) + " totalqueries "
            + StringFromVal(myStats->total_queries)+ " mspassed " + StringFromVal((int)myStats->mspassed) + "\n";
    cerr << output;
    exit(EXIT_SUCCESS);
}

//function is called when a worker must terminate
//it outputs its statistics and exits
static void __attribute__((noreturn))
signal_handler(int signum) {

    //sleep for random interval to avoid races at writing out results
    string res = "worker " + StringFromVal(myStats->worker) + " received signal \n";
    cerr << res;
    workerFinish();

}


static void
workerJob(EDBProxy * cl, int index, const TestConfig & tc, int logFreq) {

    string workload = workloads[index];
    //execute on the workload
    cerr << "in child workload file <" << workload << "> \n";

    Timer t;

    myStats = new Stats();
    myStats->worker = index;

    Connect * conn = new Connect(tc.host, tc.user, tc.pass, tc.db, tc.port);

    runQueriesFromFile(cl, workload, true, false, conn, "", true, false, myStats, logFreq);

    cerr << "Done on " << workload << "\n";
    myStats->mspassed = t.lap_ms();

    workerFinish();

}

static void runExp(EDBProxy * cl, int noWorkers, const TestConfig & tc, int logFreq) {

        assert_s(system("rm -f pieces/result") >= 0, "problem removing pieces/result");
        assert_s(system("touch pieces/result;") >= 0, "problem creating pieces/result");

    resultFile = "pieces/exp_result";


    assert_s(signal(SIGTERM, signal_handler) != SIG_ERR ,"signal could not set the handler");

    int childstatus;
    int index;
    int i;
    pid_t pids[noWorkers];

    double interval, querytput, querylat;
    int allQueries, allQueriesOK;

    for (i = 0; i < noWorkers; i++) {
        index = i;
        pid_t pid = fork();
        if (pid == 0) {
            workerJob(cl, index, tc, logFreq);

        } else if (pid < 0) {
            cerr << "failed to fork \n";
            exit(1);
        } else { // in parent
            pids[i] = pid;
        }
    }

    //wait until any child finishes

    pid_t firstchild = waitpid(-1, &childstatus,0);

    assert_s(WIFEXITED(childstatus), "the first child returning terminated abnormally\n");

    cerr << "One child finished, stopping all other children..\n";

    //signal the other children to stop
    for (int j = 0; j < noWorkers; j++) {
        if (pids[j] != firstchild) {
            assert_s(kill(pids[j], SIGTERM) == 0, "could not send signal to child " + StringFromVal(j));
        }
    }

    //now wait and make sure all finished
    for (int j = 0; j < noWorkers; j++) {
        if (pids[j] != firstchild) {
            if (waitpid(pids[j], &childstatus, 0) == -1) {
                cerr << "there were problems with process " << j << "\n";
            }
        }

    }



    interval = 0.0;
    querytput = 0.0; querylat = 0.0;
    allQueries = 0; allQueriesOK = 0;


    for (i = 0; i < noWorkers; i++) {

        ifstream resultFileIn(resultFile+StringFromVal(i));

        if (!resultFileIn.is_open()) {
            cerr << "cannot open results file to read\n";
            exit(1);
        }
        int worker, queries_failed, total_queries;
        double mspassed;

        resultFileIn >> worker;
        resultFileIn >> queries_failed;
        resultFileIn >> total_queries;
        resultFileIn >> mspassed;

        resultFileIn.close();

        allQueries += total_queries;
        allQueriesOK += total_queries - queries_failed;
        interval = max(interval, mspassed);//there should be just one worker reporting nonzero time

        cerr << "parent: worker " << worker << ": mspased " << mspassed << " totalqueries " <<
                total_queries << " queriesfailed " << queries_failed <<
                " \n";

    }



    querytput = allQueriesOK*1000.0/interval;
    querylat = interval * noWorkers/allQueries;
    cerr <<"overall:  throughput " << querytput << " queries/sec latency " << querylat << " msec/query \n";

    char * ev = getenv("RUNEXP_LOG_RESPONSE");
    if (ev != NULL) {
        //let's log the response
        ofstream f(ev);

        f << querytput << "\n";
        f << querylat << "\n";

        f.close();
    }

}
*/

static void
loadDB(const TestConfig & tc, string dbname, string dumpname) {
    string comm = "mysql -u root -pletmein -e 'drop database if exists " + dbname + "; ' ";
    cerr << comm << "\n";
    assert_s(system(comm.c_str()) >= 0, "cannot drop db with if exists");

    comm = "mysql -u root -pletmein -e 'create database " + dbname + ";' ";
    cerr << comm << "\n";
    assert_s(system(comm.c_str()) >= 0, "cannot create db");

    if (dumpname != "") {
        comm = "mysql -u root -pletmein " + dbname + " < " + tc.edbdir  + "/../eval/dumps/" + dumpname + "; ";
        cerr << comm << "\n";
        assert_s(system(comm.c_str()) >= 0, "cannotload dump");
    }
}

static void
startProxy(const TestConfig & tc, string host, uint port) {

    setenv("CRYPTDB_LOG", cryptdb_logger::getConf().c_str(), 1);
    setenv("CRYPTDB_MODE", "single", 1);


    pid_t proxy_pid = fork();
    if (proxy_pid == 0) {
        LOG(test) << "starting proxy, pid " << getpid();

        stringstream script_path, address, backend;
        script_path << "--proxy-lua-script=" << tc.edbdir << "/../mysqlproxy/wrapper.lua";
        address << "--proxy-address=localhost:" << port;
        backend << "--proxy-backend-addresses=" << host << ":" << tc.port;

        cerr << "starting proxy on port " << port << "\n";
        cerr << "\n";
    cerr << "mysql-proxy" << " --plugins=proxy" <<
                " --max-open-files=1024 " <<
                script_path.str().c_str() << " " <<
                address.str().c_str() <<
      backend.str().c_str() << "\n";
    cerr << "\n";
        execlp("mysql-proxy",
                "mysql-proxy", "--plugins=proxy",
                "--max-open-files=1024",
                script_path.str().c_str(),
                address.str().c_str(),
                backend.str().c_str(),
                (char *) 0);
        LOG(warn) << "could not execlp: " << strerror(errno);
        exit(-1);
    } else if (proxy_pid < 0) {
        assert_s(false,"failed to fork");
    }

    //back in parent, wait for proxy to start
    cerr << "waiting for proxy to start\n";
    sleep(1);
}




static void
testTrace(const TestConfig &tc, int argc, char ** argv)
{
    cerr << "testTrace uses old EDBProxy -- not run" << endl;    
    /*
    string masterKey =  BytesFromInt(mkey, AES_KEY_BYTES);

    //trace encrypt_db createsfile indexfile queriesfile insertsfile outputfile

    EDBProxy * cl = NULL;


    if (argc < 2) {
        cerr << "usage: test trace encrypt_queries/eval\n";
        return;
    }

    if (string(argv[1]) == "encrypt_queries") {
        if (argc != 9) {
            cerr << "trace encrypt_queries createsfile indexfile queriestotrainfile queriestotranslate baseoutputfile totallines"
                    " noWorkers \n";
            return;
        }

        cl = new EDBProxy(tc.host, tc.user, tc.pass, tc.db, tc.port);
        cl->setMasterKey(masterKey);


        string queriestotranslate = argv[5];
        string baseoutputfile = argv[6];
        int totalLines = atoi(argv[7]);
        int noWorkers = atoi(argv[8]);

        dotrain(cl, argv[2], argv[4], "0");
        assignWork(queriestotranslate, noWorkers, totalLines, 1, true);

        pid_t pids[noWorkers];
        for (int i = 0; i < noWorkers; i++) {
            int index = i;
            string work = workloads[index];
            string outputfile = baseoutputfile + fixedRepr(index);
            assert_s(system(("rm -f " + outputfile).c_str()) >= 0, "failed to remove " + outputfile);
            pid_t pid = fork();
            if (pid == 0) {
                Stats * st = new Stats();
                runQueriesFromFile(cl, work, false, true, NULL, outputfile, false, false, st, 100000);

                cerr << "worker " << i << "finished\n";
                exit(1);
            } else if (pid < 0) {
                cerr << "failed to fork \n";
                exit(1);
            } else { // in parent
                pids[i] = pid;
            }
        }

        int childstatus;
        for (int i = 0; i < noWorkers; i++) {
            if (waitpid(pids[i], &childstatus, 0) == -1) {
                cerr << "there were problems with process " << pids[i]
                                                                    << "\n";
            }
        }

        return;
    };

    if (string(argv[1]) == "eval") {
        if (argc != 8) {
            cerr << "trace eval queryfile totallines noworkers noRepeats split? logFreq \n";
            return;
        }

        cl = new EDBProxy(tc.host, tc.user, tc.pass, tc.db, tc.port);
        cl->setMasterKey(masterKey);


        string queryfile = argv[2];
        int totalLines = atoi(argv[3]);
        int noWorkers = atoi(argv[4]);
        int noRepeats = atoi(argv[5]);
        bool split = atoi(argv[6]);
        int logFreq = atoi(argv[7]);

        assignWork(queryfile, noWorkers, totalLines, noRepeats, split);

        runExp(cl, noWorkers, tc, logFreq);

        delete cl;

        return;
    }

    if (string(argv[1]) == "latency") {
        if (argc < 5) {
            cerr << "trace latency queryfile logFreq serverhost [optional: enctablesfile] \n";
            return;
        }

        string queryfile = argv[2];
        int logFreq = atoi(argv[3]);
        string serverhost = argv[4];
        string filename = "";

        if (argc == 6) {
            filename = argv[5];
        }

        //LOAD DUMP
        if (serverhost == "localhost") {
            loadDB(tc, "tpccenc", "up_dump_enc_w1");
        }

        //START PROXY
        setenv("CRYPTDB_DB", "tpccenc", 1);
        setenv("EDBDIR", tc.edbdir.c_str(), 1);
        string tpccdir = tc.edbdir + "/../eval/tpcc/";
        if (filename != "") {
            setenv("LOAD_ENC_TABLES", filename.c_str(), 1);
        }
        setenv("TRAIN_QUERY", ("train 1 " + tpccdir +"sqlTableCreates " + tpccdir + "querypatterns_bench 0").c_str(), 1);
        uint proxy_port = 5333;
        int res = system("killall mysql-proxy;");
        cerr << "killing proxy .. " << res << "\n";
        sleep(2);
        startProxy(tc, serverhost , proxy_port);


        Connect * conn = new Connect(tc.host, tc.user, tc.pass, "tpccenc", proxy_port);

        Stats * stats = new Stats();
        runQueriesFromFile(NULL, queryfile, 1, 0, conn, "", false, false, stats, logFreq);

        cerr << "latency " << stats->mspassed/stats->total_queries << "\n";

        delete cl;

        return;

    }


    */
    return;
}

static void
generateEncTables(const TestConfig & tc, int argc, char ** argv)
{
    cerr << "generateEncTables uses old EDBProxy -- not run" << endl;
    /*
    if (argc!=2) {
        cerr << "usage: gen_enc_tables filename\n";
        return;
    }

    cerr << "generating enc tables for tpcc \n";

    string filename = argv[1];

    string masterKey =  BytesFromInt(mkey, AES_KEY_BYTES);

    EDBProxy * cl;

    cl = new EDBProxy(tc.host, tc.user, tc.pass, tc.db, tc.port);
    cl->setMasterKey(masterKey);

    cl->execute("train 1 ../eval/tpcc/sqlTableCreates ../eval/tpcc/querypatterns_bench 0");

    cerr << "a\n";
    list<OPESpec> opes = {
            {"new_order.no_o_id", 0, 11000},
            {"oorder.o_id", 0, 11000},
            {"order_line.ol_o_id", 0, 10000},
            {"stock.s_quantity", 0, 100}
    };
    cerr << "b\n";
    cl->generateEncTables(opes, 0, 10000, 20000, filename);

    delete cl;
    */
}

static void
testEncTables(const TestConfig & tc, int argc, char ** argv) {
    cerr << "testEncTables uses old EDBProxy -- not run" << endl;
    /*if (argc!=3) {
        cerr << "usage: test_enc_tables filename queriesfile\n";
        return;
    }

    cerr << "loading enc tables for tpcc \n";

    string filename = argv[1];
    string queryfile = argv[2];

    string masterKey =  BytesFromInt(mkey, AES_KEY_BYTES);

    EDBProxy * cl;

    assert_s(system("mysql -u root -pletmein -e 'drop database if exists tpccenc;' ") >= 0, "cannot drop tpccenc with if exists");
    assert_s(system("mysql -u root -pletmein -e 'create database tpccenc;' ") >= 0, "cannot create tpccenc");

    cl = new EDBProxy(tc.host, tc.user, tc.pass, "tpccenc", tc.port);
    cl->setMasterKey(masterKey);

    cl->execute("train 1 ../eval/tpcc/sqlTableCreates ../eval/tpcc/querypatterns_bench 0");

    cerr << "loading enc tables from file " << filename << "..";
    cl->loadEncTables(filename);
    cerr << "done! \n";

    cerr << "load dump\n";

    assert_s(system("mysql -u root -pletmein tpccenc < ../eval/dumps/up_dump_enc_w1") >=0, "cannot load dump");

    Connect * conn = new Connect(tc.host, tc.user, tc.pass, "tpccenc", tc.port);

    Stats * stats = new Stats();
    runQueriesFromFile(cl, queryfile, 1, 1, conn, "", 0, false, stats, 1000);
    */
}


/*
cerr << "trying to execute " << (path+"loadData.sh") << " " << "mysqlproxy.properties" << "\n";
execlp((path+"loadData.sh").c_str(),
"loadData.sh", "mysqlproxy.properties",
 "numWarehouses",
 "4",
 (char *) 0
 );

LOG(warn) << "could not execlp bench: " << strerror(errno);

 */


static void
testBench(const TestConfig & tc, int argc, char ** argv)
{
    if (argc < 2) {
        cerr << "usage: bench logplain or [client,server,both]";
        return;
    }

    if (string(argv[1]) == "logplain") {

        if (argc != 3){
            cerr << "usage: bench logplain nowarehouses;";
            return;
        }

        string numWarehouses = argv[2];

        loadDB(tc, "cryptdbtest", "");

        string comm = "mysql -u root -pletmein cryptdbtest < ../../tpcc/orig_table_creates ;";
        cerr << comm << "\n";
        assert_s(system(comm.c_str()) >= 0, "cannot create tables");

        setenv("CRYPTDB_MODE", "single", 1);
        setenv("DO_CRYPT", "false", 1);
        //setenv("EXECUTE_QUERIES", "false", 1);
        setenv("LOG_PLAIN_QUERIES", (string("plain_insert_w")+numWarehouses).c_str(), 1);
        setenv("CRYPTDB_DB", "cryptdbtest", 1);
        setenv("EDBDIR", tc.edbdir.c_str(), 1);
        setenv("CRYPTDB_LOG", cryptdb_logger::getConf().c_str(), 1);

        string tpccdir = tc.edbdir + "/../eval/tpcc/";

        int res = system("killall mysql-proxy;");
        sleep(2);
        cerr << "killing proxies .. " <<res << "\n";
        //start proxy(ies)
        startProxy(tc, "127.0.0.1", 5133);

        comm = string("java -cp  ../build/classes:../lib/edb-jdbc14-8_0_3_14.jar:../lib/ganymed-ssh2-build250.jar:") +
                   "../lib/hsqldb.jar:../lib/mysql-connector-java-5.1.10-bin.jar:../lib/ojdbc14-10.2.jar:../lib/postgresql-8.0.309.jdbc3.jar " +
                   "-Ddriver=com.mysql.jdbc.Driver " +
                   "-Dconn=jdbc:mysql://localhost:5133/cryptdbtest " +
                   "-Duser=root -Dpassword=letmein LoadData.LoadData numWarehouses " + numWarehouses;
        cerr << "\n" << comm << "\n\n";
        assert_s(system(comm.c_str())>=0, "problem running benchmark");

        return;
    }

    if((argc != 3) && (argc != 9)) {
        cerr << "usage: bench role[client, server, both] encrypted?[1,0] [specify for client: proxyhost serverhost noWorkers oneProxyPerWorker[1/0] noWarehouses timeLimit(mins)]    \n";
        exit(-1);
    }

    string role = argv[1];
    bool encrypted = atoi(argv[2]);

    bool is_client = false;
    bool is_server = false;

    uint baseport = 5133;


    if (role == "both") {
        is_client = true;
        is_server = true;
    } else {
        if (role == "server") {
            is_server = true;
        } else
            {
            if (role != "client") {
                cerr <<"invalid role\n";
            } else {
                is_client = true;
            }
            }
    }

    string serverhost = "localhost", proxyhost = "localhost", timeLimit="", noWarehouses="";
    bool oneProxyPerWorker = 0;
    unsigned int noWorkers = 0;

    if (is_client) {
        proxyhost = argv[3];
        serverhost = argv[4];
        noWorkers = atoi(argv[5]);
        oneProxyPerWorker = atoi(argv[6]);
        assert_s(oneProxyPerWorker == 0 || oneProxyPerWorker == 1, "oneProxyPerWorker should be 0 or 1");
        noWarehouses = argv[7];
        timeLimit = argv[8];
    }

    if (encrypted) {

        if (is_server) {
            loadDB(tc, "tpccenc", "up_dump_enc_w1");

        }

        if (is_client) {
            setenv("EDBDIR", tc.edbdir.c_str(), 1);
            setenv("CRYPTDB_LOG", cryptdb_logger::getConf().c_str(), 1);
            setenv("CRYPTDB_MODE", "single", 1);
            setenv("CRYPTDB_DB", "tpccenc", 1);
        string tpccdir = tc.edbdir + "/../eval/tpcc/";
            setenv("LOAD_ENC_TABLES", (tpccdir+"enc_tables_w1").c_str(), 1);

            //configure proxy
            setenv("TRAIN_QUERY", ("train 1 " + tpccdir +"sqlTableCreates " + tpccdir + "querypatterns_bench 0").c_str(), 1);

            int res = system("killall mysql-proxy;");
            sleep(2);
            cerr << "killing proxies .. " <<res << "\n";
            //start proxy(ies)
            if (!oneProxyPerWorker) {
                startProxy(tc, serverhost, baseport);
            } else {
                for (unsigned int i = 0 ; i < noWorkers; i++) {
                    startProxy(tc, serverhost, baseport + i);
                }
            }

           string comm = string("java") + " -cp  ../build/classes:../lib/edb-jdbc14-8_0_3_14.jar:../lib/ganymed-ssh2-build250.jar:" +
                   "../lib/hsqldb.jar:../lib/mysql-connector-java-5.1.10-bin.jar:../lib/ojdbc14-10.2.jar:../lib/postgresql-8.0.309.jdbc3.jar " +
                   "-Ddriver=com.mysql.jdbc.Driver " +
                   "-Dconn=jdbc:mysql://"+proxyhost+":"+StringFromVal(baseport)+"/tpccenc " +
                   "-Duser=root -Dpassword=letmein " +
                   "-Dnwarehouses="+noWarehouses+" -Dnterminals=" + StringFromVal(noWorkers)+
                   " -DtimeLimit="+timeLimit+" client.jTPCCHeadless";
           cerr << "\n" << comm << "\n\n";
           assert_s(system(comm.c_str())>=0, "problem running benchmark");
        }
    } else {

        if (is_server) {
            loadDB(tc, "tpccplain", "dump_plain_w8");
        }
        //just start the benchmark
        if (is_client) {
            assert_s(system((string("java") + " -cp  ../build/classes:../lib/edb-jdbc14-8_0_3_14.jar:../lib/ganymed-ssh2-build250.jar:"
                    "../lib/hsqldb.jar:../lib/mysql-connector-java-5.1.10-bin.jar:../lib/ojdbc14-10.2.jar:../lib/postgresql-8.0.309.jdbc3.jar "
                    "-Ddriver=com.mysql.jdbc.Driver "
                    "-Dconn=jdbc:mysql://"+serverhost+":3306/tpccplain "
                    "-Duser=root -Dpassword=letmein "
                    "-Dnwarehouses="+noWarehouses+" -Dnterminals=" + StringFromVal(noWorkers) +
                    " -DtimeLimit="+timeLimit+" client.jTPCCHeadless").c_str())>=0,
                    "problem running benchmark");
        }
    }

    //don't forget to create indexes!

}



/*
    bool outputOnions = false;
    if (argc == 6) {
        outputOnions = argv[4];
    }
 */

// cl->plain_execute("DROP TABLE IF EXISTS
// phpbb_acl_groups,phpbb_acl_options,phpbb_acl_roles,
// phpbb_acl_roles_data,phpbb_acl_users,phpbb_attachments,
// phpbb_banlist,phpbb_bbcodes,phpbb_bookmarks,phpbb_bots,
// phpbb_config,phpbb_confirm,phpbb_disallow,phpbb_drafts,
// phpbb_extension_groups,phpbb_extensions,phpbb_forums,
// phpbb_forums_access,phpbb_forums_track,phpbb_forums_watch,
// phpbb_groups,phpbb_icons,phpbb_lang,phpbb_log,phpbb_moderator_cache,
// phpbb_modules,phpbb_poll_options,phpbb_poll_votes,phpbb_posts,
// phpbb_privmsgs,phpbb_privmsgs_folder,phpbb_privmsgs_rules,
// phpbb_privmsgs_to,phpbb_profile_fields,phpbb_profile_fields_data,
// phpbb_profile_fields_lang,phpbb_profile_lang,phpbb_ranks,
// phpbb_reports,phpbb_reports_reasons,phpbb_search_results,
// phpbb_search_wordlist,phpbb_search_wordmatch,phpbb_sessions,
// phpbb_sessions_keys,phpbb_sitelist,phpbb_smilies,phpbb_styles,
// phpbb_styles_imageset,phpbb_styles_imageset_data,
// phpbb_styles_template,phpbb_styles_template_data,phpbb_styles_theme,
// phpbb_topics,phpbb_topics_posted,phpbb_topics_track,
// phpbb_topics_watch,phpbb_user_group,phpbb_users,
// phpbb_warnings,phpbb_words,phpbb_zebra;");
/*
    ifstream createsfile(argv[2]);
    ifstream tracefile(argv[3]);
    string query;

    int noinstr = atoi(argv[4]);

    if (!createsfile.is_open()) {
        cerr << "cannot open " << argv[2] << "\n";
        exit(1);
    }

    while (!createsfile.eof()) {
        getline(createsfile, query);
        cerr << "line is < " << query << ">\n";
        if (query.length() < 3) { continue; }
        //list<string> q = cl->rewriteEncryptQuery(query, rb);
        //assert_s(q.size() == 1, "query translated has more than one query or
        // no queries;");
        list<string> res = cl->rewriteEncryptQuery(query);
        //cerr << "problem with query!\n";
        assert_s(res.size() == 1, "query did not return one");
        //cout << q.front() << "\n";

    }
    cerr << "creates ended \n";

    if (!tracefile.is_open()) {
        cerr << "cannot open " << argv[3] << "\n";
        exit(1);
    }

    struct timeval starttime, endtime;

    gettimeofday(&starttime, NULL);

    for (int i = 0; i <noinstr; i++) {

        if (!tracefile.eof()) {
            getline(tracefile, query);
            //if ((i<1063000) && (i>17)) {continue;}
            //cerr << "line is < " << query << ">\n";
            if (i % 100 == 0) {cerr << i << "\n"; }
            if (query.length() < 3) { continue; }
            //list<string> q = cl->rewriteEncryptQuery(query,
            // rb);
            //assert_s(q.size() == 1, "query translated has more than one
            // query or no queries;");
            if (!cl->execute(query).ok) {
                cerr << "problem with query!\n";
            }
            //cout << q.front() << "\n";
        } else {
            cerr << "instructions ended \n";
            break;
        }
    }

    gettimeofday(&endtime, NULL);

    cout << (noinstr*1.0/timeInSec(starttime,endtime)) << "\n";

    //cerr << "DONE with trace \n";

    if (outputOnions) {
        cerr << "outputting state of onions ... \n";
        cl->outputOnionState();
    }

    tracefile.close();
 */



static void
test_PKCS(const TestConfig &tc, int ac, char **av)
{

    PKCS * pk,* sk;
    CryptoManager::generateKeys(pk, sk);
    assert_s(pk != NULL, "pk is null");
    assert_s(sk != NULL, "pk is null");

    string pkbytes = CryptoManager::marshallKey(pk, true);

    assert_s(pkbytes ==
            CryptoManager::marshallKey(CryptoManager::unmarshallKey(pkbytes,
                    1),
                    1), "marshall does not work");

    string skbytes = CryptoManager::marshallKey(sk, false);

    assert_s(skbytes ==
            CryptoManager::marshallKey(CryptoManager::unmarshallKey(skbytes,
                    0),
                    0), "marshall does not work");

    char msg[] = "Hello world";

    string enc = CryptoManager::encrypt(pk, msg);
    string dec = CryptoManager::decrypt(sk, enc);
    assert_s(msg == dec, "decryption is not original msg");

    cerr << "msg" << dec << "\n";
}

static void help(const TestConfig &tc, int ac, char **av);

static struct {
    const char *name;
    const char * description;
    void (*f)(const TestConfig &, int ac, char **av);
} tests[] = {
        { "access",         "",                             &TestAccessManager::run },
        { "access_old",     "",                             &accessManagerTest },
        { "aes",            "",                             &evaluate_AES },
        { "autoinc",        "",                             &autoIncTest },
        //{ "consider",       "consider queries (or not)",    &TestNotConsider::run },
        { "crypto",         "crypto functions",             &TestCrypto::run },
        //{ "multi",          "integration multi principal",  &TestMultiPrinc::run },
        { "paillier",       "",                             &testPaillier },
        { "parseaccess",    "",                             &testParseAccess },
        { "pkcs",           "",                             &test_PKCS },
        { "proxy",          "proxy",                        &TestProxy::run },
        { "queries",        "queries",                      &TestQueries::run },
        { "shell",          "interactive shell",            &interactiveTest },
        //{ "single",         "integration - single principal",&TestSinglePrinc::run },
        { "gen_enc_tables", "",                             &generateEncTables },
        { "test_enc_tables","",                             &testEncTables },
        { "trace",          "trace eval",                   &testTrace },
        { "bench",          "TPC-C benchmark eval",         &testBench },
        { "utils",          "",                             &testUtils },
        { "train",          "",                             &testTrain },

        { "help",             "",                           &help },
};

static void
help(const TestConfig &tc, int ac, char **av)
{
    cerr << "Usage: " << av[0] << " [options] testname" << endl;
    cerr << "Options:" << endl
            << "    -s           stop on failure [" << tc.stop_if_fail << "]" << endl
            << "    -h host      database server [" << tc.host << "]" << endl
            << "    -t port      database port [" << tc.port << "]" << endl
            << "    -u user      database username [" << tc.user << "]" << endl
            << "    -p pass      database password [" << tc.pass << "]" << endl
            << "    -d db        database to use [" << tc.db << "]" << endl
            << "    -v group     enable verbose messages in group" << endl;
    cerr << "Verbose groups:" << endl;
    for (auto i = log_name_to_group.begin(); i != log_name_to_group.end(); i++)
        cerr << "    " << i->first << endl;
    cerr << "Supported tests:" << endl;
    for (uint i = 0; i < NELEM(tests); i++)
        cerr << "    " << left << setw(20) << tests[i].name
        << tests[i].description << endl;
}

int
main(int argc, char ** argv)
{

    TestConfig tc;
    int c;

    while ((c = getopt(argc, argv, "v:sh:u:p:d:t:")) != -1) {
        switch (c) {
        case 'v':
            if (log_name_to_group.find(optarg) == log_name_to_group.end()) {
                help(tc, argc, argv);
                exit(0);
            }

            cryptdb_logger::enable(log_name_to_group[optarg]);
            break;

        case 's':
            tc.stop_if_fail = true;
            break;

        case 'u':
            tc.user = optarg;
            break;

        case 'p':
            tc.pass = optarg;
            break;

        case 'd':
            tc.db = optarg;
            break;

        case 'h':
            tc.host = optarg;
            break;

        case 't':
            tc.port = atoi(optarg);
            break;

        default:
            help(tc, argc, argv);
            exit(0);
        }
    }

    if (argc == optind) {
        interactiveTest(tc, argc - optind, argv + optind);
        return 0;
    }

    for (uint i = 0; i < NELEM(tests); i++) {
        if (!strcasecmp(argv[optind], tests[i].name)) {
            tests[i].f(tc, argc - optind, argv + optind);
            return 0;
        }
    }

    help(tc, argc, argv);
}

/*    if (strcmp(argv[1], "train") == 0) {
                test_train();
                return 0;
        }*/
/*        if (strcmp(argv[1], "trace") == 0) {
                        if (argc != 4) { cerr << "usage ./test trace file
                           noqueries isSecure ";}
                        runTrace(argv[2], atoi(argv[3]), atoi(argv[4]));
                        return 0;
                } */ /*
        if (strcmp(argv[1], "convert") == 0) {
                convertQueries();
                return 0;
        }

        if (strcmp(argv[1], "instance") == 0) {
                createInstance();
                return 0;
        }
                 */
/*
                if (strcmp(argv[1], "convertdump") == 0) {
                        convertDump();
                        return 0;
                }
 */
/*    if (strcmp(argv[1], "load") == 0) {
                if (argc != 8) {
                        cerr << "usage: test load noWorkers totalLines logFreq
                           file workeri1 workeri2\n";
                        exit(1);
                }
                string filein = argv[5];
                cerr << "input file is " << argv[5] << "\n";
                parallelLoad(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
                   filein, atoi(argv[6]), atoi(argv[7]));
                return 0;
        }

        //int noWorkers, int noRepeats, int logFreq, int totalLines, string
           dfile, bool hasTransac
        if (strcmp(argv[1], "throughput") == 0) {

                if (argc != 9) {
                        cerr << "usage: test throughput noWorkers noRepeats
                           logFreq totalLines queryFile isSecure hasTransac
                           \n";
                        exit(1);
                }
                simpleThroughput(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
                   atoi(argv[5]), string(argv[6]), atoi(argv[7]),
                   atoi(argv[8]));
        }

        if (strcmp(argv[1], "latency") == 0) {
                        // queryFile, maxQueries, logFreq, isSecure, isVerbose
                        if (argc != 7) {
                                cerr << "usage: test latency queryFile maxQue
                                   logFreq isSecure isVerbose \n";
                                exit(1);
                        }
                        latency(string(argv[2]), atoi(argv[3]), atoi(argv[4]),
                           atoi(argv[5]), atoi(argv[6]));
        }
 */    /*
        if (strcmp(argv[1], "integration") == 0){
                testEDBProxy();
                return 0;
        }
        cerr << "unknown option\n";


        //testCryptoManager();
        //testEDBProxy();
        //tester t = tester(tc, randomBytes(AES_KEY_BYTES));
        //t.testClientParser();

        //tester t = tester(tc);
        //t.testMarshallBinary();

        //microEvaluate(argc, argv); //microEvaluate
        //test_OPE();
        //test_HGD();
        //test_EDBProxy_noSecurity();
        //evaluateMetrics(argc, argv);
  */
