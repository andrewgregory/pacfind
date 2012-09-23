#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#include <regex.h>

#include "alpm.h"
#include <alpm_list.h>

#include "pacfind.h"

alpm_list_t *all_pkgs = NULL;


size_t strtrim(char *str)
{
    unsigned char *start = str, *end = str + strlen(str);

    for(; isspace(*start) && start < end; start++);
    /*for(; end > start && isspace(*end); end--);*/
    for(; end > start && isspace(*(end - 1)); end--);

    memmove(str, start, end - start);
    *(end) = '\0';

    return end - start;
}

node_t *node_new(ntype_t type, void *left, void *right) {
    node_t *n = malloc(sizeof(node_t));
    n->type = type;
    n->left = left;
    n->right = right;
    return n;
}

void node_free(node_t *node) {
    if(node == NULL) {
        return;
    }
    switch(node->type) {
        case OP_AND:
        case OP_OR:
        case OP_XOR:
            node_free(node->left);
            node_free(node->right);
            break;
        case OP_NOT:
            node_free(node->left);
            break;
        default:
            free(node->left);
            free(node->right);
            break;
    }
    free(node);
}

void usage(const char *msg) {
    int status = 0;

    if(msg && *msg) {
        status = 1;
        printf("error: %s\n\n", msg);
    }

    puts("pacfind - an enhanced pacman package search tool"
"\n"
"    pacfind [-QS] OPTIONS -- QUERY\n"
"\n"
"    OPTIONS\n"
"        -Q     Search local packages\n"
"        -S     Search sync packages\n"
"        -i     display extra pkg info\n"
"        -q     display pkg name only\n"
"\n"
"    SYNTAX\n"
"        [field] [cmp] value\n"
"\n"
"        Fields\n"
"           (Searches -name and -desc if omitted)\n"
"            -filename\n"
"            -name\n"
"            -desc\n"
"            -version\n"
"            -url\n"
"            -builddate\n"
"            -installdate\n"
"            -packager\n"
"            -md5sum\n"
"            -sha256sum\n"
"            -arch\n"
"            -size\n"
"            -isize\n"
"            -depends\n"
"\n"
"        Cmp\n"
"           (Defaults to -re if omitted)\n"
"            -eq\n"
"            -ne\n"
"            -gt\n"
"            -ge\n"
"            -lt\n"
"            -le\n"
"            -re\n"
"            -nr\n"
"\n"
"       Join\n"
"           (Defaults to -and if omitted)\n"
"           -and\n"
"           -or\n"
"           -xor\n"
"\n"
"       Miscellaneous Operators\n"
"           -not\n"
"           -go    groups queries\n"
"           -gc    closes a query group\n"
"\n"
"   EXAMPLES\n"
"       List libreoffice packages without the language packs:\n"
"       pacfind -Q -- -name libreoffice -and -not -desc lang\n"
"\n"
"       Find broken packages:\n"
"       pacfind -QS -- -packager allan\n"
"\n"
"       Pacman style searching:\n"
"       pacfind -Q pacman mirrorlist\n"
    );

    exit(status);
}

node_t *parse_node(int argc, char **argv, int *i) {
    if(*i >= argc)
        return NULL;

    char *arg = argv[(*i)++];
    char *cmp = NULL;
    ntype_t t = CMP_DEFAULT;
    int j;

    /* handle join operators */
    for( j = 0; op_map[j].input; j++ ) {
        if( strcmp(arg, op_map[j].input) == 0 ) {
            if(*i >= argc) {
                return NULL;
            }

            return node_new(op_map[j].type, NULL, NULL);
        }
    }

    /* handle comparison operators without a field */
    for( j = 0; cmp_map[j].input; j++ ) {
        if( strcmp(arg, cmp_map[j].input) == 0 ) {
            if(*i >= argc) {
                return NULL;
            }
            t = cmp_map[j].type;
            arg = argv[(*i)++];
        }
    }

    /* Handle pacman style queries */
    if(*arg != '-') {
        node_t *n1 = node_new(t, strdup("name"), strdup(arg));
        node_t *n2 = node_new(t, strdup("desc"), strdup(arg));
        node_t *o1 = node_new(OP_OR, n1, n2);

        node_t *n3 = node_new(t, strdup("provides"), strdup(arg));
        node_t *n4 = node_new(t, strdup("group"), strdup(arg));
        node_t *o2 = node_new(OP_OR, n3, n4);

        return node_new(OP_OR, o1, o2);
    }

    /* remove leading '-' from field names */
    while(*arg == '-') {
        arg++;
    }

    /* Handle field comparisons */
    if(*i >= argc) {
        return NULL;
    }
    cmp = argv[(*i)++];

    for( j = 0; cmp_map[j].input && t == CMP_DEFAULT; j++ ) {
        if( strcmp(cmp, cmp_map[j].input) == 0 ) {
            if(*i >= argc) {
                return NULL;
            }

            cmp = argv[(*i)++];
            t = cmp_map[j].type;
        }
    }

    return node_new(t, strdup(arg), strdup(cmp));
}

node_t *parse_query(int argc, char **argv, int *i) {
    if(*i >= argc)
        return NULL;

    node_t *query = NULL;

    while(*i < argc) {
        node_t *node = parse_node(argc, argv, i);
        node_t *op;

        switch(node->type) {
            case OP_AND:
            case OP_OR:
            case OP_XOR:
                if(node->right) {
                    op = node_new(OP_AND, NULL, NULL);
                    break;
                }

                op = node;
                node = parse_node(argc, argv, i);

                switch(node->type) {
                    case OP_AND:
                    case OP_OR:
                    case OP_XOR:
                    case OP_GROUP_CLOSE:
                        usage("Invalid query string.");
                        break;
                    case OP_NOT:
                        node->left = parse_node(argc, argv, i);
                        break;
                    case OP_GROUP_OPEN:
                        node = parse_query(argc, argv, i);
                        break;
                    default:
                        break;
                }
                break;

                break;
            case OP_NOT:
                op = node_new(OP_AND, NULL, NULL);
                node->left = parse_node(argc, argv, i);
                break;
            case OP_GROUP_OPEN:
                op = node_new(OP_AND, NULL, NULL);
                node = parse_query(argc, argv, i);
                break;
            case OP_GROUP_CLOSE:
                return query;
                break;
            default:
                op = node_new(OP_AND, NULL, NULL);
                break;
        }

        op->left = query;
        op->right = node;
        query = op;
        (*i)++;
    }

    return query;
}

int parse_opts(int argc, char **argv, config_t *config) {
    int option_index = 0;
    int qs_passed = 0;
    int c;

    static struct option long_options[] = {
        {"help"       , no_argument       , NULL , 'h'} ,
        {"sync"       , no_argument       , NULL , 'S'} ,
        {"query"      , no_argument       , NULL , 'Q'} ,
        {"deps"       , no_argument       , NULL , 'd'} ,
        {"explicit"   , no_argument       , NULL , 'e'} ,
        {"unrequired" , no_argument       , NULL , 't'} ,
        {"upgrades"   , no_argument       , NULL , 'u'} ,
        {"search"     , no_argument       , NULL , 's'} ,
        {"info"       , no_argument       , NULL , 'i'} ,
        {"quiet"      , no_argument       , NULL , 'q'} ,
        {"foreign"    , no_argument       , NULL , 'm'} ,
        {"list"       , optional_argument , NULL , 'l'} ,
        {"repo"       , optional_argument , NULL , 'r'} ,
        {"groups"     , required_argument , NULL , 'g'} ,
        {0, 0, 0, 0}
    };

    while((c = getopt_long(argc, argv, "+QSqdeshmtug:ilr",
                    long_options, &option_index)) != -1) {

        switch(c) {
            case 0:
                break;
            case 'h':
                usage(NULL);
                break;
            case 'S':
                if(!qs_passed)
                    config->local = 0;
                qs_passed = 1;
                config->sync = 1;
                break;
            case 'Q':
                if(!qs_passed)
                    config->sync = 0;
                qs_passed = 1;
                config->local = 1;
                break;
            case 'd':
                config->depends = 1;
                break;
            case 'q':
                config->quiet = 1;
                break;
            case 'e':
                config->explicit = 1;
                break;
            case 'm':
                config->foreign = 1;
                break;
            case 't':
                config->unneeded = 1;
                break;
            case 'u':
                config->upgrades = 1;
                break;
            case 'g':
                /* config->groups->append(optarg) */
                break;
            case 'i':
                config->info_level++;
                break;
            case 'l':
            case 'r':
                /* config->repos->append(optarg) */
                break;
            case 's':
                break;
            default:
                break;
        }
    }
    return optind;
}

int pcmp(const void *p1, const void *p2) {
    if(p1 == p2) {
        return 0;
    }
    if(p1 < p2) {
        return -1;
    }
    return 1;
}

typedef int (*cmp_fn) (const void *, const void *);
typedef char* (*prop_fn) (const void *);
typedef alpm_list_t* (*list_fn) (const void *);
typedef int (*eq_fn) (int);

int regex_cmp(const char *val, regex_t *re) {
    return regexec(re, val, 0, 0, 0);
}

int intcmp(long *i1, long *i2) { return *i2 - *i1; }

int eq(int i) { return i == 0; }
int ne(int i) { return i != 0; }
int gt(int i) { return i > 0; }
int ge(int i) { return i >= 0; }
int lt(int i) { return i < 0; }
int le(int i) { return i <= 0; }

char *alpm_dep_get_name(alpm_depend_t *dep) { return dep->name; }

alpm_list_t *get_pkgs(char *selector, alpm_pkg_t *pkg, alpm_list_t *ret) {
    char *end = strchr(selector, '.');
    int recursive = 0;
    field_t f;
    list_fn lfn = NULL;
    prop_fn pfn = (prop_fn) alpm_dep_compute_string;
    int need_deep_free = 0;

    if(!end || end == selector) {
        return ret;
    }

    if(*(end - 1) == '%') {
        recursive = 1;
        end--;
    }

    size_t j;
    for( j = 0; field_map[j].input; j++) {
        if( strncmp(selector, field_map[j].input, end - selector) == 0 ) {
            f = field_map[j].field;
            break;
        }
    }

    switch(f) {
        case DEPENDS:
            lfn = (list_fn) alpm_pkg_get_depends;
            break;
        case OPTDEPENDS:
            lfn = (list_fn) alpm_pkg_get_optdepends;
            pfn = (prop_fn) strdup;
            break;
        case PROVIDES:
            lfn = (list_fn) alpm_pkg_get_provides;
            break;
        case CONFLICTS:
            lfn = (list_fn) alpm_pkg_get_conflicts;
            break;
        case REPLACES:
            lfn = (list_fn) alpm_pkg_get_replaces;
            break;
        case REQUIREDBY:
            lfn = (list_fn) alpm_pkg_compute_requiredby;
            pfn = (prop_fn) strdup;
            need_deep_free = 1;
            break;
        default:
            printf("unimplemented selector: %s\n", selector);
            return ret;
            break;
    }

    alpm_list_t *d, *plist = lfn(pkg);
    for(d = plist; d; d = alpm_list_next(d) ) {
        char *dep_string = pfn ? pfn(d->data) : d->data;;
        alpm_pkg_t *s = alpm_find_satisfier(all_pkgs, dep_string);
        free(dep_string);
        if(!alpm_list_find_ptr(ret, s)) {
            ret = alpm_list_add(ret, s);
            if(recursive) {
                ret = get_pkgs(selector, s, ret);
            }
        }
    }
    if(need_deep_free) {
        FREELIST(plist);
    }

    return ret;
}

alpm_list_t *filter_pkgs(node_t *cmp, alpm_list_t *pkgs) {
    alpm_list_t *p = pkgs;
    alpm_list_t *ret = NULL;

    char *c;
    int need_deep_free = 0;

    long tmp;

    list_fn lfn = NULL;
    prop_fn pfn = NULL;
    cmp_fn cfn = (cmp_fn) strcmp;
    eq_fn efn = NULL;

    regex_t reg;

    field_t field = 0;
    char *fieldname = cmp->left;
    void *value = cmp->right;

    if((c = strchr(fieldname, '.'))) {
        cmp->left = c + 1;
        for(; pkgs; pkgs = alpm_list_next(pkgs)) {
            p = get_pkgs(fieldname, pkgs->data, NULL);
            alpm_list_t *m;
            if(p && (m = filter_pkgs(cmp, p))) {
                ret = alpm_list_add(ret, pkgs->data);
                alpm_list_free(m);
            }
            alpm_list_free(p);
        }
        cmp->left = fieldname;
        return ret;
    }

    int j;
    for( j = 0; field_map[j].input; j++) {
        if( strcmp(fieldname, field_map[j].input) == 0 ) {
            field = field_map[j].field;
            break;
        }
    }

    switch(field) {
        case FILENAME:
            pfn = (prop_fn) alpm_pkg_get_filename;
            break;
        case NAME:
            pfn = (prop_fn) alpm_pkg_get_name;
            break;
        case DESC:
            pfn = (prop_fn) alpm_pkg_get_desc;
            break;
        case VERSION:
            pfn = (prop_fn) alpm_pkg_get_version;
            cfn = (cmp_fn) alpm_pkg_vercmp;
            break;
        case URL:
            pfn = (prop_fn) alpm_pkg_get_url;
            break;
        case BUILDDATE:
            pfn = (prop_fn) alpm_pkg_get_builddate;
            cfn = (cmp_fn) intcmp;
            tmp = atol(value);
            value = &tmp;
            break;
        case INSTALLDATE:
            pfn = (prop_fn) alpm_pkg_get_installdate;
            cfn = (cmp_fn) intcmp;
            tmp = atol(value);
            value = &tmp;
            break;
        case PACKAGER:
            pfn = (prop_fn) alpm_pkg_get_packager;
            break;
        case MD5SUM:
            pfn = (prop_fn) alpm_pkg_get_md5sum;
            break;
        case SHA256SUM:
            pfn = (prop_fn) alpm_pkg_get_sha256sum;
            break;
        case ARCH:
            pfn = (prop_fn) alpm_pkg_get_arch;
            break;
        case SIZE:
            pfn = (prop_fn) alpm_pkg_get_size;
            cfn = (cmp_fn) intcmp;
            tmp = atol(value);
            value = &tmp;
            break;
        case ISIZE:
            pfn = (prop_fn) alpm_pkg_get_isize;
            cfn = (cmp_fn) intcmp;
            tmp = atol(value);
            value = &tmp;
            break;
        /*case BASE64SIG:*/
            /*pfn = (prop_fn) alpm_pkg_get_base64sig;*/
            /*cfn = (cmp_fn) strcmp;*/
            /*break;*/

        case LICENSE:
            lfn = (list_fn) alpm_pkg_get_licenses;
            pfn = NULL;
            break;
        case GROUP:
            lfn = (list_fn) alpm_pkg_get_groups;
            pfn = NULL;
            break;

        case DEPENDS:
            lfn = (list_fn) alpm_pkg_get_depends;
            pfn = (prop_fn) alpm_dep_get_name;
            break;
        case OPTDEPENDS:
            lfn = (list_fn) alpm_pkg_get_optdepends;
            pfn = NULL;
            break;
        case PROVIDES:
            lfn = (list_fn) alpm_pkg_get_provides;
            pfn = (prop_fn) alpm_dep_get_name;
            break;
        case REQUIREDBY:
            lfn = (list_fn) alpm_pkg_compute_requiredby;
            pfn = NULL;
            need_deep_free = 1;
            break;
        case CONFLICTS:
            lfn = (list_fn) alpm_pkg_get_conflicts;
            pfn = (prop_fn) alpm_dep_get_name;
            break;
        case REPLACES:
            lfn = (list_fn) alpm_pkg_get_replaces;
            pfn = (prop_fn) alpm_dep_get_name;
            break;

        default:
            printf("unimplemented field: %s\n", fieldname);
            return NULL;
            break;
    }

    if(cmp->type == CMP_DEFAULT) {
        cmp->type = CMP_RE;
    }

    switch(cmp->type) {
        case CMP_EQ:
            efn = (eq_fn) eq;
            break;
        case CMP_NE:
            efn = (eq_fn) ne;
            break;
        case CMP_GT:
            efn = (eq_fn) gt;
            break;
        case CMP_GE:
            efn = (eq_fn) ge;
            break;
        case CMP_LT:
            efn = (eq_fn) lt;
            break;
        case CMP_LE:
            efn = (eq_fn) le;
            break;
        case CMP_RE:
        case CMP_NR:
            if(regcomp(&reg, (char*) value, REG_EXTENDED | REG_NOSUB | REG_ICASE | REG_NEWLINE) != 0)
                return NULL;
            value = &reg;
            cfn = (cmp_fn) regex_cmp;
            efn = cmp->type == CMP_RE ? (eq_fn) eq : (eq_fn) ne;
            break;
        default:
            printf("field: %s\n", field);
            printf("value: %s\n", value);
            printf("bad cmp\n");
            return NULL;
            break;
    }

    if(lfn) {
        for(; p; p = alpm_list_next(p)) {
            alpm_list_t *plist = lfn(p->data);
            alpm_list_t *l;
            for(l = plist; l; l = alpm_list_next(l) ) {
                void *prop = pfn ? pfn(p->data) : l->data;
                if(efn(cfn(prop, value))) {
                    ret = alpm_list_add(ret, p->data);
                    break;
                }
            }
            if(need_deep_free) {
                FREELIST(plist);
            }
        }
    } else {
        for(; p; p = alpm_list_next(p)) {
            void *prop = pfn(p->data);

            if(prop && efn(cfn(prop, value))) {
                ret = alpm_list_add(ret, p->data);
            }
        }
    }

    if(cmp->type == CMP_RE || cmp->type == CMP_NR) {
        regfree(value);
    }

    return ret;
}

alpm_list_t *run_query(node_t *query, alpm_list_t *pkgs) {
    if( query == NULL ) {
        return alpm_list_copy(pkgs);
    }

    alpm_list_t *left;
    alpm_list_t *right;

    switch(query->type) {
        case OP_AND:
            left = run_query(query->left, pkgs);
            right = run_query(query->right, left);
            alpm_list_free(left);
            return right;
            break;
        case OP_OR:
            left = run_query(query->left, pkgs);
            right = run_query(query->right, pkgs);
            pkgs = alpm_list_join(left, right);
            return pkgs;
            break;
        case OP_XOR:
            left = run_query(query->left, pkgs);
            pkgs = alpm_list_diff(pkgs, left, pcmp);
            right = run_query(query->right, pkgs);
            alpm_list_free(left);
            alpm_list_free(pkgs);
            return right;
            break;
        case OP_NOT:
            left = run_query(query->left, pkgs);
            pkgs = alpm_list_diff(pkgs, left, pcmp);
            alpm_list_free(left);
            return pkgs;
            break;
    }

    return filter_pkgs(query, pkgs);
}

alpm_list_t *build_pkg_list(alpm_handle_t *handle, config_t *config, alpm_list_t *names) {
    alpm_list_t *pkgs = NULL;
    alpm_list_t *dblist = NULL;

    FILE *fp;
    fp = fopen("/etc/pacman.conf", "r");
    char line[512];
    const alpm_siglevel_t level = ALPM_SIG_DATABASE | ALPM_SIG_DATABASE_OPTIONAL;

    while(fgets(line, 512, fp)) {
        size_t linelen;
        char *ptr;

        if((ptr = strchr(line, '#'))) {
            *ptr = '\0';
        }

        linelen = strtrim(line);

        if(linelen < 3) {
            continue;
        }

        if(line[0] == '[' && line[linelen - 1] == ']') {
            line[linelen - 1] = '\0';
            ptr = line + 1;

            if(strcmp(ptr, "options") != 0) {
                alpm_db_register_sync(handle, ptr, level);
            }
        }
    }

    fclose(fp);

    if(config->sync && !(config->depends || config->explicit || config->unneeded || config->foreign)) {
        dblist = alpm_list_join(dblist, alpm_list_copy(alpm_option_get_syncdbs(handle)));
    }
    if(config->local) {
        dblist = alpm_list_add(dblist, alpm_option_get_localdb(handle));
    }

    alpm_list_t *d;
    for(d = dblist; d; d = alpm_list_next(d)) {
        alpm_list_t *p = alpm_db_get_pkgcache(d->data);
        if(names) {
            alpm_list_t *n;
            for(n = names; n; n = alpm_list_next(n)) {
                char *name = n->data;
                char *s = strchr(name, '/');

                if(s) {
                    if(strncmp(name, alpm_db_get_name(d->data), s - name - 1) != 0) {
                        continue;
                    }
                    name = s + 1;
                }

                alpm_list_t *p2 = p;
                for( ; p2; p2 = alpm_list_next(p2)) {
                    if(strcmp(alpm_pkg_get_name(p2->data), name) == 0) {
                        pkgs = alpm_list_add(pkgs, p2->data);
                    }
                }
            }
        }
        else {
            for( ; p; p = alpm_list_next(p)) {
                if(!names || alpm_list_find_str(names, alpm_pkg_get_name(p->data))) {
                    pkgs = alpm_list_add(pkgs, p->data);
                }
            }
        }
    }

    alpm_list_free(dblist);

    return pkgs;
}

void dump_pkg_short(alpm_pkg_t *pkg, int verbosity) {
    if(verbosity < 0) {
        puts(alpm_pkg_get_name(pkg));
    } else {
        printf("%s/%s %s\n", alpm_db_get_name(alpm_pkg_get_db(pkg)),
                alpm_pkg_get_name(pkg), alpm_pkg_get_version(pkg));
        printf("    %s\n", alpm_pkg_get_desc(pkg));
    }
}

void dump_pkg_full(alpm_pkg_t *pkg, int verbosity) {
    dump_pkg_short(pkg, verbosity);
}

void print_pkgs(alpm_list_t *pkgs, config_t *config) {
    int verbosity = config ? config->info_level - config->quiet : 0;
    if(pkgs == NULL) {
        return;
    }

    pkgs = alpm_list_remove_dupes(pkgs);

    alpm_list_t *i;
    if(config && config->info_level) {
        for(i = pkgs; i; i = alpm_list_next(i)) {
            dump_pkg_full(i->data, verbosity);
        }
    } else {
        for(i = pkgs; i; i = alpm_list_next(i)) {
            dump_pkg_short(i->data, verbosity);
        }
    }
    alpm_list_free(pkgs);
}

int main(int argc, char **argv) {
    config_t config = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    config.local = 1;
    config.sync = 1;
    node_t *query;
    int i;
    alpm_list_t *matched = NULL;
    alpm_list_t *names = NULL;

    if(!isatty(fileno(stdin))) {
        char buffer[512];
        while(fgets(buffer, 512, stdin)) {
            if(strtrim(buffer)) {
                names = alpm_list_add(names, strdup(buffer));
            }
        }
    }

    /*alpm_errno_t err;*/

    i = parse_opts(argc, argv, &config);
    alpm_handle_t *handle = alpm_initialize("/", "/var/lib/pacman", NULL);
    query = parse_query(argc, argv, &i);
    all_pkgs = build_pkg_list(handle, &config, names);

    if(config.depends) {
        alpm_list_t *p;
        for(p = all_pkgs; p; p = alpm_list_next(p)) {
            if(alpm_pkg_get_reason(p->data) != ALPM_PKG_REASON_DEPEND) {
                all_pkgs = alpm_list_remove_item(all_pkgs, p);
            }
        }
    }

    if(config.explicit) {
        alpm_list_t *p;
        for(p = all_pkgs; p; p = alpm_list_next(p)) {
            if(alpm_pkg_get_reason(p->data) != ALPM_PKG_REASON_EXPLICIT) {
                all_pkgs = alpm_list_remove_item(all_pkgs, p);
            }
        }
    }

    if(config.unneeded) {
        alpm_list_t *p;
        for(p = all_pkgs; p; p = alpm_list_next(p)) {
            alpm_list_t *r = alpm_pkg_compute_requiredby(p->data);
            if(r) {
                all_pkgs = alpm_list_remove_item(all_pkgs, p);
                FREELIST(r);
            }
        }
    }

    if(config.foreign) {
        alpm_list_t *p, *dbs = alpm_option_get_syncdbs(handle);
        for(; dbs; dbs = alpm_list_next(dbs)) {
            for(p = all_pkgs; p; p = alpm_list_next(p)) {
                if(alpm_db_get_pkg(dbs->data, alpm_pkg_get_name(p->data))) {
                    all_pkgs = alpm_list_remove_item(all_pkgs, p);
                }
            }
        }
    }

    if(query) {
        matched = run_query(query, all_pkgs);
        node_free(query);
        print_pkgs(matched, &config);
    } else {
        print_pkgs(all_pkgs, &config);
    }

    alpm_list_free(all_pkgs);
    alpm_list_free(matched);

    alpm_release(handle);

    return 0;
}
