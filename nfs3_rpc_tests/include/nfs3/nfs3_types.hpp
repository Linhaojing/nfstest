#pragma once

#include "nfs3/nfs3_constants.hpp"
#include "nfs3/xdr_codec.hpp"
#include <vector>
#include <optional>
#include <memory>

namespace nfs3 {

struct writeverf3 {
    uint8_t data[8];

    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(data, 8);
    }

    void deserialize(xdr::XdrBuffer& buf) {
        std::vector<uint8_t> v;
        buf.unpack(v);
        if (v.size() >= 8) {
            std::copy(v.begin(), v.begin() + 8, data);
        }
    }
};

struct nfs_fh3 {
    std::vector<uint8_t> data;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(data);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(data);
    }
    
    bool is_zero_length() const { return data.empty(); }
};

struct nfstime3 {
    uint32_t seconds = 0;
    uint32_t nseconds = 0;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(seconds);
        buf.pack(nseconds);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(seconds);
        buf.unpack(nseconds);
    }
};

struct specdata3 {
    uint32_t specdata1 = 0;
    uint32_t specdata2 = 0;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(specdata1);
        buf.pack(specdata2);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(specdata1);
        buf.unpack(specdata2);
    }
};

struct fattr3 {
    ftype3 type_ = ftype3::NF3NON;
    uint32_t mode = 0;
    uint32_t nlink = 0;
    uint32_t uid = 0;
    uint32_t gid = 0;
    uint64_t size = 0;
    uint64_t used = 0;
    specdata3 rdev;
    uint64_t fsid = 0;
    uint64_t fileid = 0;
    nfstime3 atime;
    nfstime3 mtime;
    nfstime3 ctime;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(type_));
        buf.pack(mode);
        buf.pack(nlink);
        buf.pack(uid);
        buf.pack(gid);
        buf.pack(size);
        buf.pack(used);
        buf.pack(rdev);
        buf.pack(fsid);
        buf.pack(fileid);
        buf.pack(atime);
        buf.pack(mtime);
        buf.pack(ctime);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t type_val;
        buf.unpack(type_val);
        type_ = static_cast<ftype3>(type_val);
        buf.unpack(mode);
        buf.unpack(nlink);
        buf.unpack(uid);
        buf.unpack(gid);
        buf.unpack(size);
        buf.unpack(used);
        buf.unpack(rdev);
        buf.unpack(fsid);
        buf.unpack(fileid);
        buf.unpack(atime);
        buf.unpack(mtime);
        buf.unpack(ctime);
    }
};

struct post_op_attr {
    bool follow = false;
    fattr3 attributes;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(follow);
        if (follow) {
            buf.pack(attributes);
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(follow);
        if (follow) {
            buf.unpack(attributes);
        }
    }
};

struct sattr3 {
    std::optional<uint32_t> mode;
    std::optional<uint32_t> uid;
    std::optional<uint32_t> gid;
    std::optional<uint64_t> size;
    std::optional<nfstime3> atime;
    std::optional<nfstime3> mtime;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(mode.has_value() ? mode.value() : static_cast<uint32_t>(-1));
        buf.pack(uid.has_value() ? uid.value() : static_cast<uint32_t>(-1));
        buf.pack(gid.has_value() ? gid.value() : static_cast<uint32_t>(-1));
        
        bool size_set = size.has_value();
        buf.pack(size_set);
        if (size_set) {
            buf.pack(size.value());
        }
        
        bool atime_set = atime.has_value();
        buf.pack(atime_set);
        if (atime_set) {
            buf.pack(atime.value());
        }
        
        bool mtime_set = mtime.has_value();
        buf.pack(mtime_set);
        if (mtime_set) {
            buf.pack(mtime.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t val;
        buf.unpack(val);
        if (val != static_cast<uint32_t>(-1)) mode = val;
        
        buf.unpack(val);
        if (val != static_cast<uint32_t>(-1)) uid = val;
        
        buf.unpack(val);
        if (val != static_cast<uint32_t>(-1)) gid = val;
        
        bool flag;
        buf.unpack(flag);
        if (flag) {
            uint64_t sz;
            buf.unpack(sz);
            size = sz;
        }
        
        buf.unpack(flag);
        if (flag) {
            nfstime3 t;
            buf.unpack(t);
            atime = t;
        }
        
        buf.unpack(flag);
        if (flag) {
            nfstime3 t;
            buf.unpack(t);
            mtime = t;
        }
    }
};

struct wcc_attr {
    uint64_t size = 0;
    nfstime3 mtime;
    nfstime3 ctime;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(size);
        buf.pack(mtime);
        buf.pack(ctime);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(size);
        buf.unpack(mtime);
        buf.unpack(ctime);
    }
};

struct pre_op_attr {
    bool check = false;
    wcc_attr attributes;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(check);
        if (check) {
            buf.pack(attributes);
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(check);
        if (check) {
            buf.unpack(attributes);
        }
    }
};

struct wcc_data {
    pre_op_attr before;
    post_op_attr after;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(before);
        buf.pack(after);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(before);
        buf.unpack(after);
    }
};

struct SETATTR3args {
    nfs_fh3 object;
    sattr3 new_attributes;
    std::optional<nfs_fh3> guard;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(object);
        buf.pack(new_attributes);
        bool has_guard = guard.has_value();
        buf.pack(has_guard);
        if (has_guard) {
            buf.pack(guard.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(object);
        buf.unpack(new_attributes);
        bool has_guard;
        buf.unpack(has_guard);
        if (has_guard) {
            nfs_fh3 fh;
            buf.unpack(fh);
            guard = fh;
        }
    }
};

struct SETATTR3resok {
    wcc_data obj_wcc;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(obj_wcc);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(obj_wcc);
    }
};

struct SETATTR3res {
    nfsstat3 status;
    std::optional<SETATTR3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            SETATTR3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct GETATTR3args {
    nfs_fh3 object;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(object);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(object);
    }
};

struct GETATTR3resok {
    fattr3 obj_attributes;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(obj_attributes);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(obj_attributes);
    }
};

struct GETATTR3res {
    nfsstat3 status;
    std::optional<GETATTR3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            GETATTR3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct LOOKUP3args {
    nfs_fh3 what_dir;
    std::string what_name;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(what_dir);
        buf.pack(what_name);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(what_dir);
        buf.unpack(what_name);
    }
};

struct LOOKUP3resok {
    nfs_fh3 object;
    post_op_attr obj_attributes;
    post_op_attr dir_attributes;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(object);
        buf.pack(obj_attributes);
        buf.pack(dir_attributes);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(object);
        buf.unpack(obj_attributes);
        buf.unpack(dir_attributes);
    }
};

struct LOOKUP3res {
    nfsstat3 status;
    std::optional<LOOKUP3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            LOOKUP3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

enum class createmode3 : uint32_t {
    UNCHECKED = 0,
    GUARDED = 1,
    EXCLUSIVE = 2
};

struct createverf3 {
    uint8_t data[8];
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(data, 8);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        std::vector<uint8_t> v;
        buf.unpack(v);
        if (v.size() >= 8) {
            std::copy(v.begin(), v.begin() + 8, data);
        }
    }
};

struct CREATE3args {
    nfs_fh3 where_dir;
    std::string where_name;
    createmode3 how_mode;
    sattr3 how_attributes;
    createverf3 how_verf;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(where_dir);
        buf.pack(where_name);
        buf.pack(static_cast<uint32_t>(how_mode));
        buf.pack(how_attributes);
        if (how_mode == createmode3::EXCLUSIVE) {
            buf.pack(how_verf);
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(where_dir);
        buf.unpack(where_name);
        uint32_t mode_val;
        buf.unpack(mode_val);
        how_mode = static_cast<createmode3>(mode_val);
        buf.unpack(how_attributes);
        if (how_mode == createmode3::EXCLUSIVE) {
            buf.unpack(how_verf);
        }
    }
};

struct CREATE3resok {
    post_op_attr obj_attributes;
    post_op_attr dir_attributes;
    nfs_fh3 object;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(obj_attributes);
        buf.pack(dir_attributes);
        buf.pack(object);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(obj_attributes);
        buf.unpack(dir_attributes);
        buf.unpack(object);
    }
};

struct CREATE3res {
    nfsstat3 status;
    std::optional<CREATE3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            CREATE3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};
using bytes = std::vector<uint8_t>;

struct READ3args {
    nfs_fh3 file;
    uint64_t offset = 0;
    uint32_t count = 0;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(file);
        buf.pack(offset);
        buf.pack(count);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(file);
        buf.unpack(offset);
        buf.unpack(count);
    }
};

struct READ3resok {
    post_op_attr file_attributes;
    uint64_t count = 0;
    bool eof = false;
    bytes data;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(file_attributes);
        buf.pack(count);
        buf.pack(eof);
        buf.pack(data);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(file_attributes);
        buf.unpack(count);
        buf.unpack(eof);
        buf.unpack(data);
    }
};

struct READ3res {
    nfsstat3 status;
    std::optional<READ3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            READ3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct WRITE3args {
    nfs_fh3 file;
    uint64_t offset = 0;
    uint32_t count = 0;
    stable_how stable = stable_how::UNSTABLE;
    bytes data;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(file);
        buf.pack(offset);
        buf.pack(count);
        buf.pack(static_cast<uint32_t>(stable));
        buf.pack(data);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(file);
        buf.unpack(offset);
        buf.unpack(count);
        uint32_t stable_val;
        buf.unpack(stable_val);
        stable = static_cast<stable_how>(stable_val);
        buf.unpack(data);
    }
};

struct WRITE3resok {
    post_op_attr file_attributes;
    uint32_t count = 0;
    stable_how committed = stable_how::FILE_SYNC;
    writeverf3 verf;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(file_attributes);
        buf.pack(count);
        buf.pack(static_cast<uint32_t>(committed));
        buf.pack(verf);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(file_attributes);
        buf.unpack(count);
        uint32_t committed_val;
        buf.unpack(committed_val);
        committed = static_cast<stable_how>(committed_val);
        buf.unpack(verf);
    }
};

struct WRITE3res {
    nfsstat3 status;
    std::optional<WRITE3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            WRITE3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct ACCESS3args {
    nfs_fh3 object;
    uint32_t access = 0;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(object);
        buf.pack(access);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(object);
        buf.unpack(access);
    }
};

struct ACCESS3resok {
    post_op_attr obj_attributes;
    uint32_t access = 0;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(obj_attributes);
        buf.pack(access);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(obj_attributes);
        buf.unpack(access);
    }
};

struct ACCESS3res {
    nfsstat3 status;
    std::optional<ACCESS3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            ACCESS3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct READLINK3args {
    nfs_fh3 symlink;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(symlink);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(symlink);
    }
};

struct READLINK3resok {
    post_op_attr symlink_attributes;
    std::string data;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(symlink_attributes);
        buf.pack(data);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(symlink_attributes);
        buf.unpack(data);
    }
};

struct READLINK3res {
    nfsstat3 status;
    std::optional<READLINK3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            READLINK3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct MKDIR3args {
    nfs_fh3 where_dir;
    std::string where_name;
    sattr3 attributes;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(where_dir);
        buf.pack(where_name);
        buf.pack(attributes);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(where_dir);
        buf.unpack(where_name);
        buf.unpack(attributes);
    }
};

struct MKDIR3resok {
    post_op_attr obj_attributes;
    post_op_attr dir_attributes;
    nfs_fh3 object;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(obj_attributes);
        buf.pack(dir_attributes);
        buf.pack(object);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(obj_attributes);
        buf.unpack(dir_attributes);
        buf.unpack(object);
    }
};

struct MKDIR3res {
    nfsstat3 status;
    std::optional<MKDIR3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            MKDIR3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct SYMLINK3args {
    nfs_fh3 where_dir;
    std::string where_name;
    sattr3 symlink_attributes;
    std::string symlink_data;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(where_dir);
        buf.pack(where_name);
        buf.pack(symlink_attributes);
        buf.pack(symlink_data);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(where_dir);
        buf.unpack(where_name);
        buf.unpack(symlink_attributes);
        buf.unpack(symlink_data);
    }
};

struct SYMLINK3resok {
    post_op_attr obj_attributes;
    post_op_attr dir_attributes;
    nfs_fh3 object;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(obj_attributes);
        buf.pack(dir_attributes);
        buf.pack(object);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(obj_attributes);
        buf.unpack(dir_attributes);
        buf.unpack(object);
    }
};

struct SYMLINK3res {
    nfsstat3 status;
    std::optional<SYMLINK3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            SYMLINK3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

enum class ftype4 : uint32_t {
    NF4REG = 1,
    NF4DIR = 2,
    NF4BLK = 3,
    NF4CHR = 4,
    NF4LNK = 5,
    NF4SOCK = 6,
    NF4FIFO = 7,
    NF4ATTR = 8,
    NF4NAMED = 9
};

struct MKNOD3args {
    nfs_fh3 where_dir;
    std::string where_name;
    ftype4 what_type;
    sattr3 what_attributes;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(where_dir);
        buf.pack(where_name);
        buf.pack(static_cast<uint32_t>(what_type));
        buf.pack(what_attributes);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(where_dir);
        buf.unpack(where_name);
        uint32_t type_val;
        buf.unpack(type_val);
        what_type = static_cast<ftype4>(type_val);
        buf.unpack(what_attributes);
    }
};

struct MKNOD3resok {
    post_op_attr obj_attributes;
    post_op_attr dir_attributes;
    nfs_fh3 object;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(obj_attributes);
        buf.pack(dir_attributes);
        buf.pack(object);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(obj_attributes);
        buf.unpack(dir_attributes);
        buf.unpack(object);
    }
};

struct MKNOD3res {
    nfsstat3 status;
    std::optional<MKNOD3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            MKNOD3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct REMOVE3args {
    nfs_fh3 object_dir;
    std::string object_name;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(object_dir);
        buf.pack(object_name);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(object_dir);
        buf.unpack(object_name);
    }
};

struct REMOVE3resok {
    wcc_data dir_wcc;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(dir_wcc);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(dir_wcc);
    }
};

struct REMOVE3res {
    nfsstat3 status;
    std::optional<REMOVE3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            REMOVE3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct RMDIR3args {
    nfs_fh3 object_dir;
    std::string object_name;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(object_dir);
        buf.pack(object_name);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(object_dir);
        buf.unpack(object_name);
    }
};

struct RMDIR3resok {
    wcc_data dir_wcc;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(dir_wcc);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(dir_wcc);
    }
};

struct RMDIR3res {
    nfsstat3 status;
    std::optional<RMDIR3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            RMDIR3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct RENAME3args {
    nfs_fh3 from_dir;
    std::string from_name;
    nfs_fh3 to_dir;
    std::string to_name;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(from_dir);
        buf.pack(from_name);
        buf.pack(to_dir);
        buf.pack(to_name);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(from_dir);
        buf.unpack(from_name);
        buf.unpack(to_dir);
        buf.unpack(to_name);
    }
};

struct RENAME3resok {
    wcc_data fromdir_wcc;
    wcc_data todir_wcc;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(fromdir_wcc);
        buf.pack(todir_wcc);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(fromdir_wcc);
        buf.unpack(todir_wcc);
    }
};

struct RENAME3res {
    nfsstat3 status;
    std::optional<RENAME3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            RENAME3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct LINK3args {
    nfs_fh3 file;
    nfs_fh3 link_dir;
    std::string link_name;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(file);
        buf.pack(link_dir);
        buf.pack(link_name);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(file);
        buf.unpack(link_dir);
        buf.unpack(link_name);
    }
};

struct LINK3resok {
    post_op_attr file_attributes;
    wcc_data linkdir_wcc;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(file_attributes);
        buf.pack(linkdir_wcc);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(file_attributes);
        buf.unpack(linkdir_wcc);
    }
};

struct LINK3res {
    nfsstat3 status;
    std::optional<LINK3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            LINK3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct entry3 {
    uint64_t fileid = 0;
    std::string name;
    uint64_t cookie = 0;
    std::unique_ptr<entry3> nextentry;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(fileid);
        buf.pack(name);
        buf.pack(cookie);
        bool has_next = (nextentry != nullptr);
        buf.pack(has_next);
        if (has_next) {
            buf.pack(*nextentry);
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(fileid);
        buf.unpack(name);
        buf.unpack(cookie);
        bool has_next;
        buf.unpack(has_next);
        if (has_next) {
            auto entry = std::make_unique<entry3>();
            buf.unpack(*entry);
            nextentry = std::move(entry);
        }
    }
};

struct dirlist3 {
    std::unique_ptr<entry3> entries;
    bool eof = false;
    
    void serialize(xdr::XdrBuffer& buf) const {
        bool has_entries = (entries != nullptr);
        buf.pack(has_entries);
        if (has_entries) {
            buf.pack(*entries);
        }
        buf.pack(eof);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        bool has_entries;
        buf.unpack(has_entries);
        if (has_entries) {
            auto e = std::make_unique<entry3>();
            buf.unpack(*e);
            entries = std::move(e);
        }
        buf.unpack(eof);
    }
};

struct READDIR3args {
    nfs_fh3 dir;
    uint64_t cookie = 0;
    uint64_t cookieverf = 0;
    uint32_t count = 0;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(dir);
        buf.pack(cookie);
        buf.pack(cookieverf);
        buf.pack(count);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(dir);
        buf.unpack(cookie);
        buf.unpack(cookieverf);
        buf.unpack(count);
    }
};

struct READDIR3resok {
    post_op_attr dir_attributes;
    uint64_t cookieverf = 0;
    dirlist3 reply;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(dir_attributes);
        buf.pack(cookieverf);
        buf.pack(reply);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(dir_attributes);
        buf.unpack(cookieverf);
        buf.unpack(reply);
    }
};

struct READDIR3res {
    nfsstat3 status;
    std::optional<READDIR3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            READDIR3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct entryplus3 {
    uint64_t fileid = 0;
    std::string name;
    uint64_t cookie = 0;
    nfs_fh3 name_handle;
    post_op_attr name_attributes;
    std::unique_ptr<entryplus3> nextentry;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(fileid);
        buf.pack(name);
        buf.pack(cookie);
        buf.pack(name_handle);
        buf.pack(name_attributes);
        bool has_next = (nextentry != nullptr);
        buf.pack(has_next);
        if (has_next) {
            buf.pack(*nextentry);
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(fileid);
        buf.unpack(name);
        buf.unpack(cookie);
        buf.unpack(name_handle);
        buf.unpack(name_attributes);
        bool has_next;
        buf.unpack(has_next);
        if (has_next) {
            auto entry = std::make_unique<entryplus3>();
            buf.unpack(*entry);
            nextentry = std::move(entry);
        }
    }
};

struct dirlistplus3 {
    std::unique_ptr<entryplus3> entries;
    bool eof = false;
    
    void serialize(xdr::XdrBuffer& buf) const {
        bool has_entries = (entries != nullptr);
        buf.pack(has_entries);
        if (has_entries) {
            buf.pack(*entries);
        }
        buf.pack(eof);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        bool has_entries;
        buf.unpack(has_entries);
        if (has_entries) {
            auto e = std::make_unique<entryplus3>();
            buf.unpack(*e);
            entries = std::move(e);
        }
        buf.unpack(eof);
    }
};

struct READDIRPLUS3args {
    nfs_fh3 dir;
    uint64_t cookie = 0;
    uint64_t cookieverf = 0;
    uint32_t dircount = 0;
    uint32_t maxcount = 0;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(dir);
        buf.pack(cookie);
        buf.pack(cookieverf);
        buf.pack(dircount);
        buf.pack(maxcount);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(dir);
        buf.unpack(cookie);
        buf.unpack(cookieverf);
        buf.unpack(dircount);
        buf.unpack(maxcount);
    }
};

struct READDIRPLUS3resok {
    post_op_attr dir_attributes;
    uint64_t cookieverf = 0;
    dirlistplus3 reply;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(dir_attributes);
        buf.pack(cookieverf);
        buf.pack(reply);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(dir_attributes);
        buf.unpack(cookieverf);
        buf.unpack(reply);
    }
};

struct READDIRPLUS3res {
    nfsstat3 status;
    std::optional<READDIRPLUS3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            READDIRPLUS3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct FSSTAT3args {
    nfs_fh3 fsroot;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(fsroot);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(fsroot);
    }
};

struct FSSTAT3resok {
    post_op_attr obj_attributes;
    uint64_t tbytes = 0;
    uint64_t fbytes = 0;
    uint64_t abytes = 0;
    uint64_t tfiles = 0;
    uint64_t ffiles = 0;
    uint64_t afiles = 0;
    uint64_t invarsec = 0;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(obj_attributes);
        buf.pack(tbytes);
        buf.pack(fbytes);
        buf.pack(abytes);
        buf.pack(tfiles);
        buf.pack(ffiles);
        buf.pack(afiles);
        buf.pack(invarsec);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(obj_attributes);
        buf.unpack(tbytes);
        buf.unpack(fbytes);
        buf.unpack(abytes);
        buf.unpack(tfiles);
        buf.unpack(ffiles);
        buf.unpack(afiles);
        buf.unpack(invarsec);
    }
};

struct FSSTAT3res {
    nfsstat3 status;
    std::optional<FSSTAT3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            FSSTAT3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct FSINFO3args {
    nfs_fh3 fsroot;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(fsroot);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(fsroot);
    }
};

struct fsinfo3_resok_property {
    uint32_t linkmax = 0;
    uint32_t name_max = 0;
    bool no_trunc = false;
    bool chown_restricted = false;
    bool case_insensitive = false;
    bool case_preserving = false;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(linkmax);
        buf.pack(name_max);
        buf.pack(no_trunc);
        buf.pack(chown_restricted);
        buf.pack(case_insensitive);
        buf.pack(case_preserving);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(linkmax);
        unpack(name_max);
        buf.unpack(no_trunc);
        buf.unpack(chown_restricted);
        buf.unpack(case_insensitive);
        buf.unpack(case_preserving);
    }
private:
    void unpack(uint32_t& /*val*/) {  } 
};

struct FSINFO3resok {
    post_op_attr obj_attributes;
    uint32_t rtmax = 0;
    uint32_t rtpref = 0;
    uint32_t rtmult = 0;
    uint32_t wtmax = 0;
    uint32_t wtpref = 0;
    uint32_t wtmult = 0;
    uint32_t dtpref = 0;
    uint64_t maxfilesize = 0;
    uint64_t time_delta_seconds = 0;
    uint32_t time_delta_nseconds = 0;
    fsinfo3_resok_property properties;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(obj_attributes);
        buf.pack(rtmax);
        buf.pack(rtpref);
        buf.pack(rtmult);
        buf.pack(wtmax);
        buf.pack(wtpref);
        buf.pack(wtmult);
        buf.pack(dtpref);
        buf.pack(maxfilesize);
        buf.pack(time_delta_seconds);
        buf.pack(time_delta_nseconds);
        buf.pack(properties);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(obj_attributes);
        buf.unpack(rtmax);
        buf.unpack(rtpref);
        buf.unpack(rtmult);
        buf.unpack(wtmax);
        buf.unpack(wtpref);
        buf.unpack(wtmult);
        buf.unpack(dtpref);
        buf.unpack(maxfilesize);
        buf.unpack(time_delta_seconds);
        buf.unpack(time_delta_nseconds);
        buf.unpack(properties);
    }
};

struct FSINFO3res {
    nfsstat3 status;
    std::optional<FSINFO3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            FSINFO3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct PATHCONF3args {
    nfs_fh3 object;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(object);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(object);
    }
};

struct pathconf3_resok_info {
    int32_t linkmax = 0;
    int32_t name_max = 0;
    bool no_trunc = false;
    bool chown_restricted = false;
    bool case_insensitive = false;
    bool case_preserving = false;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(linkmax);
        buf.pack(name_max);
        buf.pack(no_trunc);
        buf.pack(chown_restricted);
        buf.pack(case_insensitive);
        buf.pack(case_preserving);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(linkmax);
        buf.unpack(name_max);
        buf.unpack(no_trunc);
        buf.unpack(chown_restricted);
        buf.unpack(case_insensitive);
        buf.unpack(case_preserving);
    }
};

struct PATHCONF3resok {
    post_op_attr obj_attributes;
    pathconf3_resok_info info;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(obj_attributes);
        buf.pack(info);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(obj_attributes);
        buf.unpack(info);
    }
};

struct PATHCONF3res {
    nfsstat3 status;
    std::optional<PATHCONF3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            PATHCONF3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

struct COMMIT3args {
    nfs_fh3 file;
    uint64_t offset = 0;
    uint32_t count = 0;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(file);
        buf.pack(offset);
        buf.pack(count);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(file);
        buf.unpack(offset);
        buf.unpack(count);
    }
};

struct COMMIT3resok {
    post_op_attr file_wcc;
    writeverf3 verf;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(file_wcc);
        buf.pack(verf);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(file_wcc);
        buf.unpack(verf);
    }
};

struct COMMIT3res {
    nfsstat3 status;
    std::optional<COMMIT3resok> resok;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(static_cast<uint32_t>(status));
        if (status == nfsstat3::NFS3_OK && resok.has_value()) {
            buf.pack(resok.value());
        }
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        uint32_t status_val;
        buf.unpack(status_val);
        status = static_cast<nfsstat3>(status_val);
        if (status == nfsstat3::NFS3_OK) {
            COMMIT3resok res;
            buf.unpack(res);
            resok.emplace(std::move(res));
        }
    }
};

} 
