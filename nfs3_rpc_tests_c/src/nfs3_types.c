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

/* --- RMDIR helpers --- */

void rmdir3args_init(RMDIR3args_t* args) {
    memset(args, 0, sizeof(*args));
    nfs_fh3_init(&args->object_dir);
}

void rmdir3args_destroy(RMDIR3args_t* args) {
    nfs_fh3_destroy(&args->object_dir);
    free(args->object_name);
    memset(args, 0, sizeof(*args));
}

void rmdir3args_set_name(RMDIR3args_t* args, const char* name) {
    free(args->object_name);
    args->object_name = name ? strdup(name) : NULL;
}

/* --- RENAME helpers --- */

void rename3args_init(RENAME3args_t* args) {
    memset(args, 0, sizeof(*args));
    nfs_fh3_init(&args->from_dir);
    nfs_fh3_init(&args->to_dir);
}

void rename3args_destroy(RENAME3args_t* args) {
    nfs_fh3_destroy(&args->from_dir);
    free(args->from_name);
    nfs_fh3_destroy(&args->to_dir);
    free(args->to_name);
    memset(args, 0, sizeof(*args));
}

void rename3args_set_from_name(RENAME3args_t* args, const char* name) {
    free(args->from_name);
    args->from_name = name ? strdup(name) : NULL;
}

void rename3args_set_to_name(RENAME3args_t* args, const char* name) {
    free(args->to_name);
    args->to_name = name ? strdup(name) : NULL;
}

/* --- LINK helpers --- */

void link3args_init(LINK3args_t* args) {
    memset(args, 0, sizeof(*args));
    nfs_fh3_init(&args->file);
    nfs_fh3_init(&args->link_dir);
}

void link3args_destroy(LINK3args_t* args) {
    nfs_fh3_destroy(&args->file);
    nfs_fh3_destroy(&args->link_dir);
    free(args->link_name);
    memset(args, 0, sizeof(*args));
}

void link3args_set_name(LINK3args_t* args, const char* name) {
    free(args->link_name);
    args->link_name = name ? strdup(name) : NULL;
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

/* --- SYMLINK helpers --- */

void symlink3args_init(SYMLINK3args_t* args) {
    memset(args, 0, sizeof(*args));
    nfs_fh3_init(&args->where_dir);
    sattr3_init(&args->symlink_attributes);
}

void symlink3args_destroy(SYMLINK3args_t* args) {
    nfs_fh3_destroy(&args->where_dir);
    free(args->where_name);
    free(args->symlink_data);
    memset(args, 0, sizeof(*args));
}

void symlink3args_set_name(SYMLINK3args_t* args, const char* name) {
    free(args->where_name);
    args->where_name = name ? strdup(name) : NULL;
}

void symlink3args_set_data(SYMLINK3args_t* args, const char* data) {
    free(args->symlink_data);
    args->symlink_data = data ? strdup(data) : NULL;
}

void symlink3resok_init(SYMLINK3resok_t* res) {
    memset(res, 0, sizeof(*res));
    nfs_fh3_init(&res->object);
}

void symlink3resok_destroy(SYMLINK3resok_t* res) {
    nfs_fh3_destroy(&res->object);
    memset(res, 0, sizeof(*res));
}

void symlink3res_init(SYMLINK3res_t* res) {
    memset(res, 0, sizeof(*res));
    symlink3resok_init(&res->resok);
}

void symlink3res_destroy(SYMLINK3res_t* res) {
    symlink3resok_destroy(&res->resok);
    memset(res, 0, sizeof(*res));
}

/* --- MKNOD helpers --- */

void mknod3args_init(MKNOD3args_t* args) {
    memset(args, 0, sizeof(*args));
    nfs_fh3_init(&args->where_dir);
    sattr3_init(&args->what_attributes);
}

void mknod3args_destroy(MKNOD3args_t* args) {
    nfs_fh3_destroy(&args->where_dir);
    free(args->where_name);
    memset(args, 0, sizeof(*args));
}

void mknod3args_set_name(MKNOD3args_t* args, const char* name) {
    free(args->where_name);
    args->where_name = name ? strdup(name) : NULL;
}

void mknod3resok_init(MKNOD3resok_t* res) {
    memset(res, 0, sizeof(*res));
    nfs_fh3_init(&res->object);
}

void mknod3resok_destroy(MKNOD3resok_t* res) {
    nfs_fh3_destroy(&res->object);
    memset(res, 0, sizeof(*res));
}

void mknod3res_init(MKNOD3res_t* res) {
    memset(res, 0, sizeof(*res));
    mknod3resok_init(&res->resok);
}

void mknod3res_destroy(MKNOD3res_t* res) {
    mknod3resok_destroy(&res->resok);
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

/* --- READDIRPLUS helpers --- */

void entryplus3_init(entryplus3_t* e) {
    memset(e, 0, sizeof(*e));
    nfs_fh3_init(&e->name_handle);
}

void entryplus3_destroy(entryplus3_t* e) {
    free(e->name);
    nfs_fh3_destroy(&e->name_handle);
    if (e->nextentry) {
        entryplus3_destroy(e->nextentry);
        free(e->nextentry);
    }
    memset(e, 0, sizeof(*e));
}

void entryplus3_set_name(entryplus3_t* e, const char* name) {
    free(e->name);
    e->name = name ? strdup(name) : NULL;
}

void dirlistplus3_init(dirlistplus3_t* dl) {
    memset(dl, 0, sizeof(*dl));
}

void dirlistplus3_destroy(dirlistplus3_t* dl) {
    if (dl->entries) {
        entryplus3_destroy(dl->entries);
        free(dl->entries);
    }
    memset(dl, 0, sizeof(*dl));
}

void readdirplus3resok_init(READDIRPLUS3resok_t* res) {
    memset(res, 0, sizeof(*res));
    dirlistplus3_init(&res->reply);
}

void readdirplus3resok_destroy(READDIRPLUS3resok_t* res) {
    dirlistplus3_destroy(&res->reply);
    memset(res, 0, sizeof(*res));
}

void readdirplus3res_init(READDIRPLUS3res_t* res) {
    memset(res, 0, sizeof(*res));
    readdirplus3resok_init(&res->resok);
}

void readdirplus3res_destroy(READDIRPLUS3res_t* res) {
    readdirplus3resok_destroy(&res->resok);
    memset(res, 0, sizeof(*res));
}
