#include "nfs3_c/nfs3_types.h"

void nfs_fh3_init(nfs_fh3_t* fh) {
    memset(fh, 0, sizeof(*fh));
}

void nfs_fh3_destroy(nfs_fh3_t* fh) {
    free(fh->data);
    memset(fh, 0, sizeof(*fh));
}

void nfs_fh3_set(nfs_fh3_t* fh, const uint8_t* buf, uint32_t len) {
    free(fh->data);
    fh->data = NULL;
    fh->len = 0;
    fh->cap = 0;
    if (len > 0) {
        fh->data = (uint8_t*)malloc(len);
        memcpy(fh->data, buf, len);
        fh->len = len;
        fh->cap = len;
    }
}

int nfs_fh3_is_empty(const nfs_fh3_t* fh) {
    return fh->len == 0;
}

void fattr3_init(fattr3_t* attr) {
    memset(attr, 0, sizeof(*attr));
}

void sattr3_init(sattr3_t* s) {
    memset(s, 0, sizeof(*s));
}

/* --- LOOKUP helpers --- */

void lookup3args_init(LOOKUP3args_t* args) {
    memset(args, 0, sizeof(*args));
    nfs_fh3_init(&args->what_dir);
}

void lookup3args_destroy(LOOKUP3args_t* args) {
    nfs_fh3_destroy(&args->what_dir);
    free(args->what_name);
    memset(args, 0, sizeof(*args));
}

void lookup3args_set_name(LOOKUP3args_t* args, const char* name) {
    free(args->what_name);
    args->what_name = name ? strdup(name) : NULL;
}

void lookup3resok_init(LOOKUP3resok_t* res) {
    memset(res, 0, sizeof(*res));
    nfs_fh3_init(&res->object);
}

void lookup3resok_destroy(LOOKUP3resok_t* res) {
    nfs_fh3_destroy(&res->object);
    memset(res, 0, sizeof(*res));
}

void lookup3res_init(LOOKUP3res_t* res) {
    memset(res, 0, sizeof(*res));
    lookup3resok_init(&res->resok);
}

void lookup3res_destroy(LOOKUP3res_t* res) {
    lookup3resok_destroy(&res->resok);
    memset(res, 0, sizeof(*res));
}

/* --- READ helpers --- */

void read3resok_init(READ3resok_t* res) {
    memset(res, 0, sizeof(*res));
}

void read3resok_destroy(READ3resok_t* res) {
    free(res->data);
    memset(res, 0, sizeof(*res));
}

void read3res_init(READ3res_t* res) {
    memset(res, 0, sizeof(*res));
    read3resok_init(&res->resok);
}

void read3res_destroy(READ3res_t* res) {
    read3resok_destroy(&res->resok);
    memset(res, 0, sizeof(*res));
}

/* --- WRITE helpers --- */

void write3args_init(WRITE3args_t* args) {
    memset(args, 0, sizeof(*args));
    nfs_fh3_init(&args->file);
}

void write3args_destroy(WRITE3args_t* args) {
    nfs_fh3_destroy(&args->file);
    free(args->data);
    memset(args, 0, sizeof(*args));
}

/* --- CREATE helpers --- */

void create3args_init(CREATE3args_t* args) {
    memset(args, 0, sizeof(*args));
    nfs_fh3_init(&args->where_dir);
    sattr3_init(&args->how_attributes);
}

void create3args_destroy(CREATE3args_t* args) {
    nfs_fh3_destroy(&args->where_dir);
    free(args->where_name);
    memset(args, 0, sizeof(*args));
}

void create3args_set_name(CREATE3args_t* args, const char* name) {
    free(args->where_name);
    args->where_name = name ? strdup(name) : NULL;
}

void create3resok_init(CREATE3resok_t* res) {
    memset(res, 0, sizeof(*res));
    nfs_fh3_init(&res->object);
}

void create3resok_destroy(CREATE3resok_t* res) {
    nfs_fh3_destroy(&res->object);
    memset(res, 0, sizeof(*res));
}

void create3res_init(CREATE3res_t* res) {
    memset(res, 0, sizeof(*res));
    create3resok_init(&res->resok);
}

void create3res_destroy(CREATE3res_t* res) {
    create3resok_destroy(&res->resok);
    memset(res, 0, sizeof(*res));
}

/* --- REMOVE helpers --- */

void remove3args_init(REMOVE3args_t* args) {
    memset(args, 0, sizeof(*args));
    nfs_fh3_init(&args->object_dir);
}

void remove3args_destroy(REMOVE3args_t* args) {
    nfs_fh3_destroy(&args->object_dir);
    free(args->object_name);
    memset(args, 0, sizeof(*args));
}

void remove3args_set_name(REMOVE3args_t* args, const char* name) {
    free(args->object_name);
    args->object_name = name ? strdup(name) : NULL;
}

/* --- READLINK helpers --- */

void readlink3resok_init(READLINK3resok_t* res) {
    memset(res, 0, sizeof(*res));
}

void readlink3resok_destroy(READLINK3resok_t* res) {
    free(res->data);
    memset(res, 0, sizeof(*res));
}

void readlink3res_init(READLINK3res_t* res) {
    memset(res, 0, sizeof(*res));
    readlink3resok_init(&res->resok);
}

void readlink3res_destroy(READLINK3res_t* res) {
    readlink3resok_destroy(&res->resok);
    memset(res, 0, sizeof(*res));
}

/* --- MKDIR helpers --- */

void mkdir3args_init(MKDIR3args_t* args) {
    memset(args, 0, sizeof(*args));
    nfs_fh3_init(&args->where_dir);
    sattr3_init(&args->attributes);
}

void mkdir3args_destroy(MKDIR3args_t* args) {
    nfs_fh3_destroy(&args->where_dir);
    free(args->where_name);
    memset(args, 0, sizeof(*args));
}

void mkdir3args_set_name(MKDIR3args_t* args, const char* name) {
    free(args->where_name);
    args->where_name = name ? strdup(name) : NULL;
}

void mkdir3resok_init(MKDIR3resok_t* res) {
    memset(res, 0, sizeof(*res));
    nfs_fh3_init(&res->object);
}

void mkdir3resok_destroy(MKDIR3resok_t* res) {
    nfs_fh3_destroy(&res->object);
    memset(res, 0, sizeof(*res));
}

void mkdir3res_init(MKDIR3res_t* res) {
    memset(res, 0, sizeof(*res));
    mkdir3resok_init(&res->resok);
}

void mkdir3res_destroy(MKDIR3res_t* res) {
    mkdir3resok_destroy(&res->resok);
    memset(res, 0, sizeof(*res));
}

/* --- READDIR helpers --- */

void entry3_init(entry3_t* e) {
    memset(e, 0, sizeof(*e));
}

void entry3_destroy(entry3_t* e) {
    free(e->name);
    if (e->nextentry) {
        entry3_destroy(e->nextentry);
        free(e->nextentry);
    }
    memset(e, 0, sizeof(*e));
}

void entry3_set_name(entry3_t* e, const char* name) {
    free(e->name);
    e->name = name ? strdup(name) : NULL;
}

void dirlist3_init(dirlist3_t* dl) {
    memset(dl, 0, sizeof(*dl));
}

void dirlist3_destroy(dirlist3_t* dl) {
    if (dl->entries) {
        entry3_destroy(dl->entries);
        free(dl->entries);
    }
    memset(dl, 0, sizeof(*dl));
}

void readdir3resok_init(READDIR3resok_t* res) {
    memset(res, 0, sizeof(*res));
    dirlist3_init(&res->reply);
}

void readdir3resok_destroy(READDIR3resok_t* res) {
    dirlist3_destroy(&res->reply);
    memset(res, 0, sizeof(*res));
}

void readdir3res_init(READDIR3res_t* res) {
    memset(res, 0, sizeof(*res));
    readdir3resok_init(&res->resok);
}

void readdir3res_destroy(READDIR3res_t* res) {
    readdir3resok_destroy(&res->resok);
    memset(res, 0, sizeof(*res));
}
