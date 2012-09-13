#ifndef PACFIND_H
#define PACFIND_H

typedef enum field_t {
    FILENAME,
    NAME,
    DESC,
    VERSION,
    ORIGIN,
    REASON,
    LICENSE,
    GROUP,

    DEPENDS,
    OPTDEPENDS,
    CONFLICTS,
    PROVIDES,
    REPLACES,
    REQUIREDBY,

    DELTAS,
    FILES,
    BACKUP,
    DB,
    VALIDATION,
    URL,
    BUILDDATE,
    INSTALLDATE,
    PACKAGER,
    MD5SUM,
    SHA256SUM,
    ARCH,
    SIZE,
    ISIZE,
    BASE64SIG
} field_t;

typedef struct field_map_t {
    char *input;
    field_t field;
} field_map_t;

static field_map_t field_map[] = {
    {"filename", FILENAME},
    {"name", NAME},
    {"desc", DESC},
    {"version", VERSION},

    {"license", LICENSE},
    {"group", GROUP},

    {"depends", DEPENDS},
    {"optdepends", OPTDEPENDS},
    {"conflicts", CONFLICTS},
    {"provides", PROVIDES},
    {"replaces", REPLACES},
    {"requiredby", REQUIREDBY},

    {"url", URL},
    {"builddate", BUILDDATE},
    {"installdate", INSTALLDATE},
    {"packager", PACKAGER},
    {"md5sum", MD5SUM},
    {"sha256sum", SHA256SUM},
    {"arch", ARCH},
    {"size", SIZE},
    {"isize", ISIZE},
    {"base64sig", BASE64SIG},

    {NULL, 0}
};

typedef struct config_t {
    int info_level;
    int depends;
    int explicit;
    int unneeded;
    int quiet;
    int sync;
    int local;
    int foreign;
    int upgrades;
} config_t;

typedef enum ntype_t {
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_NOT,

    /* these should not exist in a query tree */
    OP_GROUP_OPEN,
    OP_GROUP_CLOSE,

    CMP_EQ,
    CMP_NE,
    CMP_GT,
    CMP_GE,
    CMP_LT,
    CMP_LE,
    CMP_RE,
    CMP_NR,

    CMP_DEFAULT
} ntype_t;

typedef struct input_map_t {
    char *input;
    ntype_t type;
} input_map_t;

typedef struct node_t {
    ntype_t type;
    void *left;
    void *right;
} node_t;

static input_map_t op_map[] = {
    {"-and", OP_AND},
    {"-or",  OP_OR},
    {"-xor", OP_XOR},
    {"-not", OP_NOT},
    {"-go",  OP_GROUP_OPEN},
    {"-gc",  OP_GROUP_CLOSE},

    {"&",  OP_AND},
    {"|",  OP_OR},
    {"^",  OP_XOR},
    {"!",  OP_NOT},
    {"(",  OP_GROUP_OPEN},
    {")",  OP_GROUP_CLOSE},

    {NULL, 0}
};

static input_map_t cmp_map[] = {
    {"-eq", CMP_EQ},
    {"-ne", CMP_NE},
    {"-gt", CMP_GT},
    {"-ge", CMP_GE},
    {"-lt", CMP_LT},
    {"-le", CMP_LE},
    {"-re", CMP_RE},
    {"-nr", CMP_NR},

    {"==", CMP_EQ},
    {"!=", CMP_NE},
    {"<",  CMP_GT},
    {"<=", CMP_GE},
    {">",  CMP_LT},
    {">=", CMP_LE},
    {"=~", CMP_RE},
    {"!~", CMP_NR},

    {NULL, 0}
};

#endif /* PACFIND_H */
