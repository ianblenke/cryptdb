#include "EDBProxy2.h"

const bool debug = true;

extern "C" void *create_embedded_thd(int client_flag);

class mysql_thrower : public std::stringstream {
 public:
    ~mysql_thrower() __attribute__((noreturn)) {
        *this << ": " << current_thd->stmt_da->message();
        throw std::runtime_error(str());
    }
};

static void
rewrite(THD * t) {

	cerr << "t is in rewrite " << t << "\n";
	LEX *lex = t->lex;


	cout << "rewrite parsed query: " << *lex << endl;

}

static void
assemble(THD * t) {

	cerr << "assembled query" << t->lex;
}

static int
parse(THD * & t, const string & db, const string & q) {

	t->set_db(db.data(), db.length());
	mysql_reset_thd_for_next_command(t);

	char * buf = (char *)malloc(q.size() + 1);
	memcpy(buf, q.c_str(), q.size());
	buf[q.size()] = '\0';
	size_t len = q.size();

	alloc_query(t, buf, len + 1);

	Parser_state ps = Parser_state();
	if (ps.init(t, buf, len))
		mysql_thrower() << "Parser_state::init";

	if (debug) cout << "input query: " << buf << endl;

	bool error = parse_sql(t, &ps, 0);
	if (error)
		mysql_thrower() << "parse_sql";

	LEX *lex = t->lex;

	rewrite(t);

	return 0;
}


list<string>
EDBProxy2::rewriteEncryptQuery(const string & db, const string &query) {

	cerr << "A\n";
	assert(create_embedded_thd(0));
	cerr << "a.5\n";
	THD *t = current_thd;

	auto ANON = cleanup([&t]() { delete t; });
	auto ANON = cleanup([&t]() { close_thread_tables(t); });
	auto ANON = cleanup([&t]() { t->cleanup_after_query(); });
	auto ANON = cleanup([&t]() { t->end_statement(); });

	parse(t, db, query);

	if (!t) {
		cerr << "problem parsing";
		exit(-1);
	}

	rewrite(t);
	assemble(t);

	return list<string>();
}

EDBProxy2::~EDBProxy2() {

}

int
main(int ac, char **av) {

    if (ac != 2) {
        cerr << "Usage: " << av[0] << " schema-db " << endl;
        exit(1);
    }

    char dir_arg[1024];
    snprintf(dir_arg, sizeof(dir_arg), "--datadir=%s", av[1]);

    cerr << "dirarg is " << dir_arg << "\n";
    const char *mysql_av[] =
        { "progname",
          "--skip-grant-tables",
          dir_arg,
          /* "--skip-innodb", */
          /* "--default-storage-engine=MEMORY", */
          "--character-set-server=utf8",
          "--language=" MYSQL_BUILD_DIR "/sql/share/"
        };

    cerr << "before server init\n";
    assert(0 == mysql_server_init(sizeof(mysql_av) / sizeof(mysql_av[0]),
                                  (char**) mysql_av, 0));
    cerr << "server init completed \n";
    assert(0 == mysql_thread_init());
    cerr << "after the thread was init\n";


	EDBProxy2 cl;
	cl.rewriteEncryptQuery("cryptdbtest", "SELECT * from t1;");
}
