#include <libpq-fe.h>
#include <string>


#define UNUSED(x) (void)(x)


static PGconn *conn = nullptr;
static PGresult *res = nullptr;

static std::string terminate(int code) {
    std::string result = std::string("allOk");
    if (code != 0) {
        fprintf(stderr, "%s\n", PQerrorMessage(conn));
        result = std::string(PQerrorMessage(conn));
    }
    if (res != nullptr)
        PQclear(res);

    return result;
}

static void clearRes() {
    PQclear(res);
    res = nullptr;
}

static void processNotice(void *arg, const char *message) {
    UNUSED(arg);
    UNUSED(message);

}

bool connect() {

    conn = PQconnectdb("user=postgres password=12345 host=127.0.0.1 dbname=bag");
    if (PQstatus(conn) != CONNECTION_OK)
        return (false);

    PQsetNoticeProcessor(conn, processNotice, nullptr);

    char *user = PQuser(conn);
    char *db_name = PQdb(conn);
    printf("User: %s\n", user);
    printf("Database name: %s\n", db_name);

    return true;
}

std::string selectAllUsers() {
    std::string result;
    res = PQexec(conn, "SELECT * "
                       "FROM employ;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
        return terminate(1);

    int nrows = PQntuples(res);
    for (int i = 0; i < nrows; i++) {
        char *id = PQgetvalue(res, i, 0);
        char *first_name = PQgetvalue(res, i, 1);
        char *last_name = PQgetvalue(res, i, 2);
        char *position = PQgetvalue(res, i, 3);
        result += "Id: ";
        result += id;
        result += " Name: ";
        result += first_name;
        result += " LastName:";
        result += last_name;
        result += " Position: ";
        result += position;
        result += "\n";
    }

    result += "Total: " + std::to_string(nrows) + " rows\n";
    clearRes();
    return result;
}

std::string selectAllTasks() {
    std::string result;
    res = PQexec(conn, "SELECT * "
                       "FROM todo;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
        return terminate(1);


    int nrows = PQntuples(res);
    for (int i = 0; i < nrows; i++) {
        char *id = PQgetvalue(res, i, 0);
        char *id_statement = PQgetvalue(res, i, 1);
        char *id_responsible = PQgetvalue(res, i, 2);
        char *name = PQgetvalue(res, i, 3);
        char *status = PQgetvalue(res, i, 4);
        char *comment = PQgetvalue(res, i, 5);
        result += "Id: ";
        result += id;
        result += " Id statement: ";
        result += id_statement;
        result += " Id responsible: ";
        result += id_responsible;
        result += " Name ";
        result += name;
        result += " Status: ";
        result += status;
        result += " Comment: ";
        result += comment;
        result += "\n";
    }

    result += "Total: " + std::to_string(nrows) + " rows\n";
    clearRes();
    return result;
}

std::string selectMyTasks(const std::string &myName, const std::string &myLastName, int &number) {
    std::string result;
    const char *query;
    if (number == 1) {
        query =
                "SELECT todo.id, employ.first_name, employ.last_name, employ.position, todo.Name, todo.status, todo.comment"
                "FROM todo JOIN employ on id_responsible = employ.id"
                "WHERE employ.first_name = 'Kolya' and employ.last_name = 'Kholkin'";
    }
    else {

        query =
                "SELECT todo.id,employ.first_name, employ.last_name, employ.position, todo.Name, todo.status, todo.comment "
                "FROM todo JOIN employ on id_statement = employ.id "
                "WHERE employ.first_name = $1 and employ.last_name = $2;";
    }


    const char *params[2];

    params[0] = myName.c_str();
    params[1] = myLastName.c_str();
    res = PQexecParams(conn, query, 2, nullptr, params,
                       nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
        return terminate(1);

    int nrows = PQntuples(res);
    for (int i = 0; i < nrows; i++) {
        char *id = PQgetvalue(res, i, 0);
        char *first_name = PQgetvalue(res, i, 1);
        char *last_name = PQgetvalue(res, i, 2);
        char *position = PQgetvalue(res, i, 3);
        char *name = PQgetvalue(res, i, 4);
        char *status = PQgetvalue(res, i, 5);
        char *comment = PQgetvalue(res, i, 6);
        result += "Id: ";
        result += id;
        result += " FirstName: ";
        result += first_name;
        result += " LastName: ";
        result += last_name;
        result += " Position: ";
        result += position;
        result += " Name ";
        result += name;
        result += " Status: ";
        result += status;
        result += " Comment: ";
        result += comment;
        result += " Name user doing: ";
        result += "\n";
    }

    result += "Total: " + std::to_string(nrows) + " rows\n";
    clearRes();
    return result;
}


std::string selectOpenTasks() {
    std::string result;
    res = PQexec(conn, "SELECT * "
                       "FROM todo WHERE status = 'OPEN';");
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
        return terminate(1);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
        return terminate(1);

    int nrows = PQntuples(res);
    for (int i = 0; i < nrows; i++) {
        char *id = PQgetvalue(res, i, 0);
        char *id_statement = PQgetvalue(res, i, 1);
        char *id_responsible = PQgetvalue(res, i, 2);
        char *comment = PQgetvalue(res, i, 3);
        result += "Id: ";
        result += id;
        result += " IdStatement: ";
        result += id_statement;
        result += " IdResponsible: ";
        result += id_responsible;
        result += " Comment: ";
        result += comment;
        result += "\n";
    }

    result += "Total: " + std::to_string(nrows) + " rows\n";
    clearRes();
    return result;
}

std::string selectRoleByName(const std::string &myName, const std::string &myLastName) {
    std::string result;
    const char *query =
            "SELECT position FROM employ WHERE first_name = $1 and last_name = $2;";
    const char *params[2];

    params[0] = myName.c_str();
    params[1] = myLastName.c_str();
    res = PQexecParams(conn, query, 2, nullptr, params,
                       nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        return terminate(1);
    }
    int nrows = PQntuples(res);
    for (int i = 0; i < nrows; i++) {
        char *role = PQgetvalue(res, i, 0);
        result += role;
    }
    clearRes();
    return result;
}

std::string insertEmploy(const std::string &name, const std::string &last_name, const std::string &position) {
    const char *query =
            "INSERT INTO employ (first_name, last_name, position) "
            " VALUES ($1, $2, $3);";
    const char *params[3];

    params[0] = name.c_str();
    params[1] = last_name.c_str();
    params[2] = position.c_str();
    res = PQexecParams(conn, query, 3, nullptr, params,
                       nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        return terminate(1);
    }
    clearRes();
    return "Successful";
}

std::string insertTasks(const std::string &first_name,
                        const std::string &last_name,
                        const std::string &id_user_doing,
                        const std::string &name_pro,
                        const std::string &status,
                        const std::string &comment
) {
    const char *query =
            "INSERT INTO todo (id_statement, id_responsible, name ,status, comment) "
            " VALUES ((SELECT id FROM employ WHERE first_name = $1 and last_name = $2), (NULLIF($3,'null')::integer),"
            " $4, $5, $6);";
    const char *params[6];

    params[0] = first_name.c_str();
    params[1] = last_name.c_str();
    params[2] = id_user_doing.c_str();
    params[3] = name_pro.c_str();
    params[4] = status.c_str();
    params[5] = comment.c_str();

    res = PQexecParams(conn, query, 6, nullptr, params,
                       nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        return terminate(1);
    }
    clearRes();
    return "Successful";
}


std::string updateTasks(const std::string &id,
                        const std::string &status) {
    const char *query =
            "UPDATE todo set status = $2 WHERE id = $1;";
    const char *params[2];

    params[0] = id.c_str();
    params[1] = status.c_str();
    res = PQexecParams(conn, query, 2, nullptr, params,
                       nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        return terminate(1);
    }
    clearRes();
    return "Successful";
}

