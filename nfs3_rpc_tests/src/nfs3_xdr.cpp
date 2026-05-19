#include "nfs3/nfs3_xdr.hpp"
#include <cstring>

namespace nfs3 {

bool_t xdr_specdata3(XDR* xdrs, specdata3* obj) {
    if (!xdr_u_int(xdrs, &obj->specdata1)) return FALSE;
    if (!xdr_u_int(xdrs, &obj->specdata2)) return FALSE;
    return TRUE;
}

bool_t xdr_nfstime3(XDR* xdrs, nfstime3* obj) {
    if (!xdr_u_int(xdrs, &obj->seconds)) return FALSE;
    if (!xdr_u_int(xdrs, &obj->nseconds)) return FALSE;
    return TRUE;
}

bool_t xdr_writeverf3(XDR* xdrs, writeverf3* obj) {
    return xdr_opaque(xdrs, reinterpret_cast<char*>(obj->data), 8);
}

bool_t xdr_createverf3(XDR* xdrs, createverf3* obj) {
    return xdr_opaque(xdrs, reinterpret_cast<char*>(obj->data), 8);
}

bool_t xdr_nfs_fh3(XDR* xdrs, nfs_fh3* obj) {
    char* ptr = nullptr;
    u_int len = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        ptr = reinterpret_cast<char*>(obj->data.data());
        len = obj->data.size();
        if (!xdr_bytes(xdrs, &ptr, &len, 64)) return FALSE;
    } else if (xdrs->x_op == XDR_DECODE) {
        if (!xdr_bytes(xdrs, &ptr, &len, 64)) return FALSE;
        obj->data.assign(ptr, ptr + len);
        free(ptr);
    }
    return TRUE;
}

bool_t xdr_fattr3(XDR* xdrs, fattr3* obj) {
    uint32_t type_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        type_val = static_cast<uint32_t>(obj->type_);
    }
    if (!xdr_u_int(xdrs, &type_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->type_ = static_cast<ftype3>(type_val);
    }

    if (!xdr_u_int(xdrs, &obj->mode)) return FALSE;
    if (!xdr_u_int(xdrs, &obj->nlink)) return FALSE;
    if (!xdr_u_int(xdrs, &obj->uid)) return FALSE;
    if (!xdr_u_int(xdrs, &obj->gid)) return FALSE;
    if (!xdr_u_hyper(xdrs, &obj->size)) return FALSE;
    if (!xdr_u_hyper(xdrs, &obj->used)) return FALSE;
    if (!xdr_specdata3(xdrs, &obj->rdev)) return FALSE;
    if (!xdr_u_hyper(xdrs, &obj->fsid)) return FALSE;
    if (!xdr_u_hyper(xdrs, &obj->fileid)) return FALSE;
    if (!xdr_nfstime3(xdrs, &obj->atime)) return FALSE;
    if (!xdr_nfstime3(xdrs, &obj->mtime)) return FALSE;
    if (!xdr_nfstime3(xdrs, &obj->ctime)) return FALSE;

    return TRUE;
}

bool_t xdr_post_op_attr(XDR* xdrs, post_op_attr* obj) {
    uint32_t follow_val = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        follow_val = obj->follow ? 1 : 0;
    }
    if (!xdr_u_int(xdrs, &follow_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->follow = (follow_val != 0);
    }
    if (obj->follow) {
        if (!xdr_fattr3(xdrs, &obj->attributes)) return FALSE;
    }
    return TRUE;
}

bool_t xdr_wcc_attr(XDR* xdrs, wcc_attr* obj) {
    if (!xdr_u_hyper(xdrs, &obj->size)) return FALSE;
    if (!xdr_nfstime3(xdrs, &obj->mtime)) return FALSE;
    if (!xdr_nfstime3(xdrs, &obj->ctime)) return FALSE;
    return TRUE;
}

bool_t xdr_pre_op_attr(XDR* xdrs, pre_op_attr* obj) {
    uint32_t check_val = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        check_val = obj->check ? 1 : 0;
    }
    if (!xdr_u_int(xdrs, &check_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->check = (check_val != 0);
    }
    if (obj->check) {
        if (!xdr_wcc_attr(xdrs, &obj->attributes)) return FALSE;
    }
    return TRUE;
}

bool_t xdr_wcc_data(XDR* xdrs, wcc_data* obj) {
    if (!xdr_pre_op_attr(xdrs, &obj->before)) return FALSE;
    if (!xdr_post_op_attr(xdrs, &obj->after)) return FALSE;
    return TRUE;
}

bool_t xdr_sattr3(XDR* xdrs, sattr3* obj) {
    uint32_t mode_val = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        mode_val = obj->mode.has_value() ? 1 : 0;
    }
    if (!xdr_u_int(xdrs, &mode_val)) return FALSE;
    if (mode_val) {
        uint32_t m = 0;
        if (xdrs->x_op == XDR_ENCODE) {
            m = obj->mode.value();
        }
        if (!xdr_u_int(xdrs, &m)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->mode = m;
        }
    } else if (xdrs->x_op == XDR_DECODE) {
        obj->mode = std::nullopt;
    }

    uint32_t uid_val = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        uid_val = obj->uid.has_value() ? 1 : 0;
    }
    if (!xdr_u_int(xdrs, &uid_val)) return FALSE;
    if (uid_val) {
        uint32_t u = 0;
        if (xdrs->x_op == XDR_ENCODE) {
            u = obj->uid.value();
        }
        if (!xdr_u_int(xdrs, &u)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->uid = u;
        }
    } else if (xdrs->x_op == XDR_DECODE) {
        obj->uid = std::nullopt;
    }

    uint32_t gid_val = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        gid_val = obj->gid.has_value() ? 1 : 0;
    }
    if (!xdr_u_int(xdrs, &gid_val)) return FALSE;
    if (gid_val) {
        uint32_t g = 0;
        if (xdrs->x_op == XDR_ENCODE) {
            g = obj->gid.value();
        }
        if (!xdr_u_int(xdrs, &g)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->gid = g;
        }
    } else if (xdrs->x_op == XDR_DECODE) {
        obj->gid = std::nullopt;
    }

    uint32_t size_val = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        size_val = obj->size.has_value() ? 1 : 0;
    }
    if (!xdr_u_int(xdrs, &size_val)) return FALSE;
    if (size_val) {
        uint64_t s = 0;
        if (xdrs->x_op == XDR_ENCODE) {
            s = obj->size.value();
        }
        if (!xdr_u_hyper(xdrs, &s)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->size = s;
        }
    } else if (xdrs->x_op == XDR_DECODE) {
        obj->size = std::nullopt;
    }

    uint32_t atime_val = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        atime_val = obj->atime.has_value() ? 1 : 0;
    }
    if (!xdr_u_int(xdrs, &atime_val)) return FALSE;
    if (atime_val) {
        nfstime3 t{};
        if (xdrs->x_op == XDR_ENCODE) {
            t = obj->atime.value();
        }
        if (!xdr_nfstime3(xdrs, &t)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->atime = t;
        }
    } else if (xdrs->x_op == XDR_DECODE) {
        obj->atime = std::nullopt;
    }

    uint32_t mtime_val = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        mtime_val = obj->mtime.has_value() ? 1 : 0;
    }
    if (!xdr_u_int(xdrs, &mtime_val)) return FALSE;
    if (mtime_val) {
        nfstime3 t{};
        if (xdrs->x_op == XDR_ENCODE) {
            t = obj->mtime.value();
        }
        if (!xdr_nfstime3(xdrs, &t)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->mtime = t;
        }
    } else if (xdrs->x_op == XDR_DECODE) {
        obj->mtime = std::nullopt;
    }

    return TRUE;
}

static bool_t xdr_entry3_list(XDR* xdrs, entry3* first) {
    while (true) {
        uint32_t has_entry = 0;
        if (xdrs->x_op == XDR_ENCODE) {
            has_entry = (first != nullptr) ? 1 : 0;
        }
        if (!xdr_u_int(xdrs, &has_entry)) return FALSE;

        if (!has_entry) break;

        if (xdrs->x_op == XDR_DECODE) {
            first = new entry3();
            first->nextentry = nullptr;
        }

        if (!xdr_u_hyper(xdrs, &first->fileid)) return FALSE;
        char* name_ptr = nullptr;
        if (xdrs->x_op == XDR_ENCODE) {
            name_ptr = const_cast<char*>(first->name.c_str());
        }
        if (!xdr_string(xdrs, &name_ptr, 255)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            first->name = name_ptr;
            free(name_ptr);
        }
        if (!xdr_u_hyper(xdrs, &first->cookie)) return FALSE;

        if (xdrs->x_op == XDR_ENCODE) {
            first = first->nextentry.get();
        }
    }
    return TRUE;
}

static bool_t xdr_dirlist3_internal(XDR* xdrs, dirlist3* obj) {
    if (xdrs->x_op == XDR_DECODE) {
        obj->entries.reset();
    }

    entry3* first = nullptr;
    if (xdrs->x_op == XDR_ENCODE) {
        first = obj->entries.get();
    }

    if (!xdr_entry3_list(xdrs, first)) return FALSE;

    uint32_t eof_val = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        eof_val = obj->eof ? 1 : 0;
    }
    if (!xdr_u_int(xdrs, &eof_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->eof = (eof_val != 0);
    }

    return TRUE;
}

static bool_t xdr_entryplus3_list(XDR* xdrs, entryplus3* first) {
    while (true) {
        uint32_t has_entry = 0;
        if (xdrs->x_op == XDR_ENCODE) {
            has_entry = (first != nullptr) ? 1 : 0;
        }
        if (!xdr_u_int(xdrs, &has_entry)) return FALSE;

        if (!has_entry) break;

        if (xdrs->x_op == XDR_DECODE) {
            first = new entryplus3();
            first->nextentry = nullptr;
        }

        if (!xdr_u_hyper(xdrs, &first->fileid)) return FALSE;
        char* name_ptr = nullptr;
        if (xdrs->x_op == XDR_ENCODE) {
            name_ptr = const_cast<char*>(first->name.c_str());
        }
        if (!xdr_string(xdrs, &name_ptr, 255)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            first->name = name_ptr;
            free(name_ptr);
        }
        if (!xdr_u_hyper(xdrs, &first->cookie)) return FALSE;
        if (!xdr_nfs_fh3(xdrs, &first->name_handle)) return FALSE;
        if (!xdr_post_op_attr(xdrs, &first->name_attributes)) return FALSE;

        if (xdrs->x_op == XDR_ENCODE) {
            first = first->nextentry.get();
        }
    }
    return TRUE;
}

static bool_t xdr_dirlistplus3_internal(XDR* xdrs, dirlistplus3* obj) {
    if (xdrs->x_op == XDR_DECODE) {
        obj->entries.reset();
    }

    entryplus3* first = nullptr;
    if (xdrs->x_op == XDR_ENCODE) {
        first = obj->entries.get();
    }

    if (!xdr_entryplus3_list(xdrs, first)) return FALSE;

    uint32_t eof_val = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        eof_val = obj->eof ? 1 : 0;
    }
    if (!xdr_u_int(xdrs, &eof_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->eof = (eof_val != 0);
    }

    return TRUE;
}

bool_t xdr_GETATTR3args(XDR* xdrs, GETATTR3args* obj) {
    return xdr_nfs_fh3(xdrs, &obj->object);
}

bool_t xdr_GETATTR3res(XDR* xdrs, GETATTR3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        GETATTR3resok resok;
        if (xdrs->x_op == XDR_ENCODE) {
            resok = obj->resok.value();
        }
        if (!xdr_fattr3(xdrs, &resok.obj_attributes)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_SETATTR3args(XDR* xdrs, SETATTR3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->object)) return FALSE;
    if (!xdr_sattr3(xdrs, &obj->new_attributes)) return FALSE;

    uint32_t has_guard = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        has_guard = obj->guard.has_value() ? 1 : 0;
    }
    if (!xdr_u_int(xdrs, &has_guard)) return FALSE;
    if (has_guard) {
        nfs_fh3 fh;
        if (xdrs->x_op == XDR_ENCODE) {
            fh = obj->guard.value();
        }
        if (!xdr_nfs_fh3(xdrs, &fh)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->guard = fh;
        }
    } else if (xdrs->x_op == XDR_DECODE) {
        obj->guard = std::nullopt;
    }

    return TRUE;
}

bool_t xdr_SETATTR3res(XDR* xdrs, SETATTR3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        SETATTR3resok resok;
        if (xdrs->x_op == XDR_ENCODE) {
            resok = obj->resok.value();
        }
        if (!xdr_wcc_data(xdrs, &resok.obj_wcc)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_LOOKUP3args(XDR* xdrs, LOOKUP3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->what_dir)) return FALSE;
    char* name_ptr = const_cast<char*>(obj->what_name.c_str());
    if (!xdr_string(xdrs, &name_ptr, 255)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->what_name = name_ptr;
        free(name_ptr);
    }
    return TRUE;
}

bool_t xdr_LOOKUP3res(XDR* xdrs, LOOKUP3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        LOOKUP3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_nfs_fh3(xdrs, &resok.object)) return FALSE;
        if (!xdr_post_op_attr(xdrs, &resok.obj_attributes)) return FALSE;
        if (!xdr_post_op_attr(xdrs, &resok.dir_attributes)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_ACCESS3args(XDR* xdrs, ACCESS3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->object)) return FALSE;
    if (!xdr_u_int(xdrs, &obj->access)) return FALSE;
    return TRUE;
}

bool_t xdr_ACCESS3res(XDR* xdrs, ACCESS3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        ACCESS3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_post_op_attr(xdrs, &resok.obj_attributes)) return FALSE;
        if (!xdr_u_int(xdrs, &resok.access)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_READLINK3args(XDR* xdrs, READLINK3args* obj) {
    return xdr_nfs_fh3(xdrs, &obj->symlink);
}

bool_t xdr_READLINK3res(XDR* xdrs, READLINK3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        READLINK3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_post_op_attr(xdrs, &resok.symlink_attributes)) return FALSE;
        char* data_ptr = nullptr;
        if (xdrs->x_op == XDR_ENCODE) {
            data_ptr = const_cast<char*>(resok.data.c_str());
        }
        if (!xdr_string(xdrs, &data_ptr, 4096)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            resok.data = data_ptr;
            free(data_ptr);
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_READ3args(XDR* xdrs, READ3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->file)) return FALSE;
    if (!xdr_u_hyper(xdrs, &obj->offset)) return FALSE;
    if (!xdr_u_int(xdrs, &obj->count)) return FALSE;
    return TRUE;
}

bool_t xdr_READ3res(XDR* xdrs, READ3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        READ3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_post_op_attr(xdrs, &resok.file_attributes)) return FALSE;
        if (!xdr_u_hyper(xdrs, &resok.count)) return FALSE;
        uint32_t eof_val = 0;
        if (xdrs->x_op == XDR_ENCODE) {
            eof_val = resok.eof ? 1 : 0;
        }
        if (!xdr_u_int(xdrs, &eof_val)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            resok.eof = (eof_val != 0);
        }
        char* data_ptr = nullptr;
        u_int data_len = 0;
        if (xdrs->x_op == XDR_ENCODE) {
            data_ptr = reinterpret_cast<char*>(const_cast<uint8_t*>(resok.data.data()));
            data_len = resok.data.size();
        }
        if (!xdr_bytes(xdrs, &data_ptr, &data_len, 65536)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            resok.data.assign(data_ptr, data_ptr + data_len);
            free(data_ptr);
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_WRITE3args(XDR* xdrs, WRITE3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->file)) return FALSE;
    if (!xdr_u_hyper(xdrs, &obj->offset)) return FALSE;
    if (!xdr_u_int(xdrs, &obj->count)) return FALSE;
    uint32_t stable_val = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        stable_val = static_cast<uint32_t>(obj->stable);
    }
    if (!xdr_u_int(xdrs, &stable_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->stable = static_cast<stable_how>(stable_val);
    }
    char* data_ptr = reinterpret_cast<char*>(const_cast<uint8_t*>(obj->data.data()));
    u_int data_len = obj->data.size();
    if (!xdr_bytes(xdrs, &data_ptr, &data_len, 65536)) return FALSE;
    return TRUE;
}

bool_t xdr_WRITE3res(XDR* xdrs, WRITE3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        WRITE3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_post_op_attr(xdrs, &resok.file_attributes)) return FALSE;
        if (!xdr_u_int(xdrs, &resok.count)) return FALSE;
        uint32_t committed_val = 0;
        if (xdrs->x_op == XDR_ENCODE) {
            committed_val = static_cast<uint32_t>(resok.committed);
        }
        if (!xdr_u_int(xdrs, &committed_val)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            resok.committed = static_cast<stable_how>(committed_val);
        }
        if (!xdr_writeverf3(xdrs, &resok.verf)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_CREATE3args(XDR* xdrs, CREATE3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->where_dir)) return FALSE;
    char* name_ptr = const_cast<char*>(obj->where_name.c_str());
    if (!xdr_string(xdrs, &name_ptr, 255)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->where_name = name_ptr;
        free(name_ptr);
    }

    uint32_t create_mode = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        create_mode = static_cast<uint32_t>(obj->how_mode);
    }
    if (!xdr_u_int(xdrs, &create_mode)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->how_mode = static_cast<createmode3>(create_mode);
    }

    if (!xdr_sattr3(xdrs, &obj->how_attributes)) return FALSE;

    if (obj->how_mode == createmode3::EXCLUSIVE) {
        if (!xdr_createverf3(xdrs, &obj->how_verf)) return FALSE;
    }

    return TRUE;
}

bool_t xdr_CREATE3res(XDR* xdrs, CREATE3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        CREATE3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_post_op_attr(xdrs, &resok.obj_attributes)) return FALSE;
        if (!xdr_wcc_data(xdrs, &resok.dir_wcc)) return FALSE;
        if (!xdr_nfs_fh3(xdrs, &resok.object)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_MKDIR3args(XDR* xdrs, MKDIR3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->where_dir)) return FALSE;
    char* name_ptr = const_cast<char*>(obj->where_name.c_str());
    if (!xdr_string(xdrs, &name_ptr, 255)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->where_name = name_ptr;
        free(name_ptr);
    }
    if (!xdr_sattr3(xdrs, &obj->attributes)) return FALSE;
    return TRUE;
}

bool_t xdr_MKDIR3res(XDR* xdrs, MKDIR3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        MKDIR3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_post_op_attr(xdrs, &resok.obj_attributes)) return FALSE;
        if (!xdr_wcc_data(xdrs, &resok.dir_wcc)) return FALSE;
        if (!xdr_nfs_fh3(xdrs, &resok.object)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_SYMLINK3args(XDR* xdrs, SYMLINK3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->where_dir)) return FALSE;
    char* name_ptr = const_cast<char*>(obj->where_name.c_str());
    if (!xdr_string(xdrs, &name_ptr, 255)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->where_name = name_ptr;
        free(name_ptr);
    }
    if (!xdr_sattr3(xdrs, &obj->symlink_attributes)) return FALSE;
    char* data_ptr = const_cast<char*>(obj->symlink_data.c_str());
    if (!xdr_string(xdrs, &data_ptr, 4096)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->symlink_data = data_ptr;
        free(data_ptr);
    }
    return TRUE;
}

bool_t xdr_SYMLINK3res(XDR* xdrs, SYMLINK3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        SYMLINK3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_post_op_attr(xdrs, &resok.obj_attributes)) return FALSE;
        if (!xdr_wcc_data(xdrs, &resok.dir_wcc)) return FALSE;
        if (!xdr_nfs_fh3(xdrs, &resok.object)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_MKNOD3args(XDR* xdrs, MKNOD3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->where_dir)) return FALSE;
    char* name_ptr = const_cast<char*>(obj->where_name.c_str());
    if (!xdr_string(xdrs, &name_ptr, 255)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->where_name = name_ptr;
        free(name_ptr);
    }
    uint32_t type_val = 0;
    if (xdrs->x_op == XDR_ENCODE) {
        type_val = static_cast<uint32_t>(obj->what_type);
    }
    if (!xdr_u_int(xdrs, &type_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->what_type = static_cast<ftype4>(type_val);
    }
    if (!xdr_sattr3(xdrs, &obj->what_attributes)) return FALSE;
    return TRUE;
}

bool_t xdr_MKNOD3res(XDR* xdrs, MKNOD3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        MKNOD3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_post_op_attr(xdrs, &resok.obj_attributes)) return FALSE;
        if (!xdr_wcc_data(xdrs, &resok.dir_wcc)) return FALSE;
        if (!xdr_nfs_fh3(xdrs, &resok.object)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_REMOVE3args(XDR* xdrs, REMOVE3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->object_dir)) return FALSE;
    char* name_ptr = const_cast<char*>(obj->object_name.c_str());
    if (!xdr_string(xdrs, &name_ptr, 255)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->object_name = name_ptr;
        free(name_ptr);
    }
    return TRUE;
}

bool_t xdr_REMOVE3res(XDR* xdrs, REMOVE3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        REMOVE3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_wcc_data(xdrs, &resok.dir_wcc)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_RMDIR3args(XDR* xdrs, RMDIR3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->object_dir)) return FALSE;
    char* name_ptr = const_cast<char*>(obj->object_name.c_str());
    if (!xdr_string(xdrs, &name_ptr, 255)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->object_name = name_ptr;
        free(name_ptr);
    }
    return TRUE;
}

bool_t xdr_RMDIR3res(XDR* xdrs, RMDIR3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        RMDIR3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_wcc_data(xdrs, &resok.dir_wcc)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_RENAME3args(XDR* xdrs, RENAME3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->from_dir)) return FALSE;
    char* from_name_ptr = const_cast<char*>(obj->from_name.c_str());
    if (!xdr_string(xdrs, &from_name_ptr, 255)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->from_name = from_name_ptr;
        free(from_name_ptr);
    }
    if (!xdr_nfs_fh3(xdrs, &obj->to_dir)) return FALSE;
    char* to_name_ptr = const_cast<char*>(obj->to_name.c_str());
    if (!xdr_string(xdrs, &to_name_ptr, 255)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->to_name = to_name_ptr;
        free(to_name_ptr);
    }
    return TRUE;
}

bool_t xdr_RENAME3res(XDR* xdrs, RENAME3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        RENAME3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_wcc_data(xdrs, &resok.fromdir_wcc)) return FALSE;
        if (!xdr_wcc_data(xdrs, &resok.todir_wcc)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_LINK3args(XDR* xdrs, LINK3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->file)) return FALSE;
    if (!xdr_nfs_fh3(xdrs, &obj->link_dir)) return FALSE;
    char* name_ptr = const_cast<char*>(obj->link_name.c_str());
    if (!xdr_string(xdrs, &name_ptr, 255)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->link_name = name_ptr;
        free(name_ptr);
    }
    return TRUE;
}

bool_t xdr_LINK3res(XDR* xdrs, LINK3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        LINK3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_post_op_attr(xdrs, &resok.file_attributes)) return FALSE;
        if (!xdr_wcc_data(xdrs, &resok.linkdir_wcc)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_READDIR3args(XDR* xdrs, READDIR3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->dir)) return FALSE;
    if (!xdr_u_hyper(xdrs, &obj->cookie)) return FALSE;
    char* verifier_ptr = reinterpret_cast<char*>(&obj->cookieverf);
    if (!xdr_opaque(xdrs, verifier_ptr, 8)) return FALSE;
    if (!xdr_u_int(xdrs, &obj->count)) return FALSE;
    return TRUE;
}

bool_t xdr_READDIR3res(XDR* xdrs, READDIR3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        if (xdrs->x_op == XDR_ENCODE) {
            if (!obj->resok.has_value()) return FALSE;
            if (!xdr_post_op_attr(xdrs, &obj->resok->dir_attributes)) return FALSE;
            char* verifier_ptr = reinterpret_cast<char*>(&obj->resok->cookieverf);
            if (!xdr_opaque(xdrs, verifier_ptr, 8)) return FALSE;
            if (!xdr_dirlist3_internal(xdrs, &obj->resok->reply)) return FALSE;
        } else {
            READDIR3resok resok;
            if (!xdr_post_op_attr(xdrs, &resok.dir_attributes)) return FALSE;
            char* verifier_ptr = reinterpret_cast<char*>(&resok.cookieverf);
            if (!xdr_opaque(xdrs, verifier_ptr, 8)) return FALSE;
            if (!xdr_dirlist3_internal(xdrs, &resok.reply)) return FALSE;
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_READDIRPLUS3args(XDR* xdrs, READDIRPLUS3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->dir)) return FALSE;
    if (!xdr_u_hyper(xdrs, &obj->cookie)) return FALSE;
    char* verifier_ptr = reinterpret_cast<char*>(&obj->cookieverf);
    if (!xdr_opaque(xdrs, verifier_ptr, 8)) return FALSE;
    if (!xdr_u_int(xdrs, &obj->dircount)) return FALSE;
    if (!xdr_u_int(xdrs, &obj->maxcount)) return FALSE;
    return TRUE;
}

bool_t xdr_READDIRPLUS3res(XDR* xdrs, READDIRPLUS3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        if (xdrs->x_op == XDR_ENCODE) {
            if (!obj->resok.has_value()) return FALSE;
            if (!xdr_post_op_attr(xdrs, &obj->resok->dir_attributes)) return FALSE;
            char* verifier_ptr = reinterpret_cast<char*>(&obj->resok->cookieverf);
            if (!xdr_opaque(xdrs, verifier_ptr, 8)) return FALSE;
            if (!xdr_dirlistplus3_internal(xdrs, &obj->resok->reply)) return FALSE;
        } else {
            READDIRPLUS3resok resok;
            if (!xdr_post_op_attr(xdrs, &resok.dir_attributes)) return FALSE;
            char* verifier_ptr = reinterpret_cast<char*>(&resok.cookieverf);
            if (!xdr_opaque(xdrs, verifier_ptr, 8)) return FALSE;
            if (!xdr_dirlistplus3_internal(xdrs, &resok.reply)) return FALSE;
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_FSSTAT3args(XDR* xdrs, FSSTAT3args* obj) {
    return xdr_nfs_fh3(xdrs, &obj->fsroot);
}

bool_t xdr_FSSTAT3res(XDR* xdrs, FSSTAT3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        FSSTAT3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_post_op_attr(xdrs, &resok.obj_attributes)) return FALSE;
        if (!xdr_u_hyper(xdrs, &resok.tbytes)) return FALSE;
        if (!xdr_u_hyper(xdrs, &resok.fbytes)) return FALSE;
        if (!xdr_u_hyper(xdrs, &resok.abytes)) return FALSE;
        if (!xdr_u_hyper(xdrs, &resok.tfiles)) return FALSE;
        if (!xdr_u_hyper(xdrs, &resok.ffiles)) return FALSE;
        if (!xdr_u_hyper(xdrs, &resok.afiles)) return FALSE;
        if (!xdr_u_int(xdrs, &resok.invarsec)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_FSINFO3args(XDR* xdrs, FSINFO3args* obj) {
    return xdr_nfs_fh3(xdrs, &obj->fsroot);
}

bool_t xdr_FSINFO3res(XDR* xdrs, FSINFO3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        FSINFO3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_post_op_attr(xdrs, &resok.obj_attributes)) return FALSE;
        if (!xdr_u_int(xdrs, &resok.rtmax)) return FALSE;
        if (!xdr_u_int(xdrs, &resok.rtpref)) return FALSE;
        if (!xdr_u_int(xdrs, &resok.rtmult)) return FALSE;
        if (!xdr_u_int(xdrs, &resok.wtmax)) return FALSE;
        if (!xdr_u_int(xdrs, &resok.wtpref)) return FALSE;
        if (!xdr_u_int(xdrs, &resok.wtmult)) return FALSE;
        if (!xdr_u_int(xdrs, &resok.dtpref)) return FALSE;
        if (!xdr_u_hyper(xdrs, &resok.maxfilesize)) return FALSE;
        if (!xdr_u_int(xdrs, &resok.time_delta_seconds)) return FALSE;
        if (!xdr_u_int(xdrs, &resok.time_delta_nseconds)) return FALSE;
        if (!xdr_u_int(xdrs, &resok.properties)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_PATHCONF3args(XDR* xdrs, PATHCONF3args* obj) {
    return xdr_nfs_fh3(xdrs, &obj->object);
}

bool_t xdr_PATHCONF3res(XDR* xdrs, PATHCONF3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        PATHCONF3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_post_op_attr(xdrs, &resok.obj_attributes)) return FALSE;
        if (!xdr_int(xdrs, &resok.info.linkmax)) return FALSE;
        if (!xdr_int(xdrs, &resok.info.name_max)) return FALSE;
        uint32_t no_trunc_val = 0;
        if (xdrs->x_op == XDR_ENCODE) {
            no_trunc_val = resok.info.no_trunc ? 1 : 0;
        }
        if (!xdr_u_int(xdrs, &no_trunc_val)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            resok.info.no_trunc = (no_trunc_val != 0);
        }
        uint32_t chown_restricted_val = 0;
        if (xdrs->x_op == XDR_ENCODE) {
            chown_restricted_val = resok.info.chown_restricted ? 1 : 0;
        }
        if (!xdr_u_int(xdrs, &chown_restricted_val)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            resok.info.chown_restricted = (chown_restricted_val != 0);
        }
        uint32_t case_insensitive_val = 0;
        if (xdrs->x_op == XDR_ENCODE) {
            case_insensitive_val = resok.info.case_insensitive ? 1 : 0;
        }
        if (!xdr_u_int(xdrs, &case_insensitive_val)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            resok.info.case_insensitive = (case_insensitive_val != 0);
        }
        uint32_t case_preserving_val = 0;
        if (xdrs->x_op == XDR_ENCODE) {
            case_preserving_val = resok.info.case_preserving ? 1 : 0;
        }
        if (!xdr_u_int(xdrs, &case_preserving_val)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            resok.info.case_preserving = (case_preserving_val != 0);
        }
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

bool_t xdr_COMMIT3args(XDR* xdrs, COMMIT3args* obj) {
    if (!xdr_nfs_fh3(xdrs, &obj->file)) return FALSE;
    if (!xdr_u_hyper(xdrs, &obj->offset)) return FALSE;
    if (!xdr_u_int(xdrs, &obj->count)) return FALSE;
    return TRUE;
}

bool_t xdr_COMMIT3res(XDR* xdrs, COMMIT3res* obj) {
    uint32_t status_val = 0;

    if (xdrs->x_op == XDR_ENCODE) {
        status_val = static_cast<uint32_t>(obj->status);
    }
    if (!xdr_u_int(xdrs, &status_val)) return FALSE;
    if (xdrs->x_op == XDR_DECODE) {
        obj->status = static_cast<nfsstat3>(status_val);
    }

    if (obj->status == nfsstat3::NFS3_OK) {
        COMMIT3resok resok;
        if (xdrs->x_op == XDR_ENCODE && obj->resok.has_value()) {
            resok = obj->resok.value();
        }
        if (!xdr_post_op_attr(xdrs, &resok.file_wcc)) return FALSE;
        if (!xdr_writeverf3(xdrs, &resok.verf)) return FALSE;
        if (xdrs->x_op == XDR_DECODE) {
            obj->resok.emplace(std::move(resok));
        }
    }

    return TRUE;
}

}
