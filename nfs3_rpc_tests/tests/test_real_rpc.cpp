#include "nfs3/test_context.hpp"
#include "nfs3/mount_client.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <chrono>

using nfs3::NFS3TestContext;
using nfs3::MountClient;
using nfs3::MountResult;

class RealRpcTest : public ::testing::Test {
protected:
    static inline std::string server_host = "127.0.0.1";
    static inline std::string export_path = "/srv/nfs";
    static inline nfs3::nfs_fh3 root_fh;
    static inline bool has_root_fh = false;
    
    void SetUp() override {
        if (!has_root_fh) {
            MountClient mounter(server_host);
            if (!mounter.is_connected()) {
                std::cerr << "Failed to connect to MOUNT service" << std::endl;
            }
            ASSERT_TRUE(mounter.is_connected()) << "Failed to connect to MOUNT service";
            
            auto result = mounter.mnt(export_path);
            if (!result.has_value()) {
                std::cerr << "MNT call failed, error: " << static_cast<int>(result.error()) << std::endl;
            }
            ASSERT_TRUE(result.has_value()) << "MNT call failed";
            
            root_fh = result->root_handle;
            has_root_fh = true;
            
            std::cout << "Got root filehandle, size=" << root_fh.data.size() << std::endl;
        }
    }
    
    static void TearDownTestSuite() {
        if (has_root_fh) {
            MountClient mounter(server_host);
            if (mounter.is_connected()) {
                mounter.umnt(export_path);
            }
        }
    }
    
    const nfs3::nfs_fh3& root() const { return root_fh; }
};

TEST_F(RealRpcTest, MountAndGetRootHandle) {
    ASSERT_TRUE(has_root_fh);
    EXPECT_FALSE(root_fh.data.empty());
    EXPECT_GT(root_fh.data.size(), 0);
}

TEST_F(RealRpcTest, GetAttrOfRoot) {
    auto endpoint = nfs3::RPCEndpoint::create(server_host, 2049);
    ASSERT_TRUE(endpoint.is_connected());
    
    nfs3::NFS3TestClient client(std::move(endpoint));
    
    auto result = client.getattr(root());
    if (!result.has_value()) {
        std::cerr << "GETATTR failed, error: " << static_cast<int>(result.error()) << std::endl;
    }
    ASSERT_TRUE(result.has_value()) << "GETATTR on root should succeed";
    
    const auto& attrs = result.value();
    EXPECT_EQ(attrs.obj_attributes.type_, nfs3::ftype3::NF3DIR);
    EXPECT_NE(attrs.obj_attributes.mode, 0);
    
    std::cout << "Root attributes:" << std::endl;
    std::cout << "  type: " << static_cast<int>(attrs.obj_attributes.type_) << std::endl;
    std::cout << "  mode: 0" << std::oct << attrs.obj_attributes.mode << std::dec << std::endl;
    std::cout << "  nlink: " << attrs.obj_attributes.nlink << std::endl;
    std::cout << "  size: " << attrs.obj_attributes.size << std::endl;
}

TEST_F(RealRpcTest, FsInfo) {
    auto endpoint = nfs3::RPCEndpoint::create(server_host, 2049);
    ASSERT_TRUE(endpoint.is_connected());
    
    nfs3::NFS3TestClient client(std::move(endpoint));
    
    auto result = client.fsinfo(root());
    ASSERT_TRUE(result.has_value()) << "FSINFO should succeed";
    
    const auto& info = result.value();
    EXPECT_GT(info.rtmax, 0);
    EXPECT_GT(info.wtmax, 0);
    
    std::cout << "FSINFO:" << std::endl;
    std::cout << "  rtmax: " << info.rtmax << std::endl;
    std::cout << "  wtmax: " << info.wtmax << std::endl;
    std::cout << "  maxfilesize: " << info.maxfilesize << std::endl;
}

TEST_F(RealRpcTest, FsStat) {
    auto endpoint = nfs3::RPCEndpoint::create(server_host, 2049);
    ASSERT_TRUE(endpoint.is_connected());
    
    nfs3::NFS3TestClient client(std::move(endpoint));
    
    auto result = client.fsstat(root());
    ASSERT_TRUE(result.has_value()) << "FSSTAT should succeed";
    
    const auto& stat = result.value();
    std::cout << "FSSTAT:" << std::endl;
    std::cout << "  tbytes: " << stat.tbytes << std::endl;
    std::cout << "  fbytes: " << stat.fbytes << std::endl;
    std::cout << "  tfiles: " << stat.tfiles << std::endl;
    std::cout << "  ffiles: " << stat.ffiles << std::endl;
}

TEST_F(RealRpcTest, PathConf) {
    auto endpoint = nfs3::RPCEndpoint::create(server_host, 2049);
    ASSERT_TRUE(endpoint.is_connected());
    
    nfs3::NFS3TestClient client(std::move(endpoint));
    
    auto result = client.pathconf(root());
    ASSERT_TRUE(result.has_value()) << "PATHCONF should succeed";
    
    const auto& pc = result.value();
    std::cout << "PATHCONF:" << std::endl;
    std::cout << "  linkmax: " << pc.info.linkmax << std::endl;
    std::cout << "  name_max: " << pc.info.name_max << std::endl;
    std::cout << "  no_trunc: " << pc.info.no_trunc << std::endl;
}

TEST_F(RealRpcTest, AccessRoot) {
    auto endpoint = nfs3::RPCEndpoint::create(server_host, 2049);
    ASSERT_TRUE(endpoint.is_connected());
    
    nfs3::NFS3TestClient client(std::move(endpoint));
    
    uint32_t access_mask = nfs3::NFS3_ACCESS_READ | nfs3::NFS3_ACCESS_LOOKUP;
    auto result = client.access(root(), access_mask);
    ASSERT_TRUE(result.has_value()) << "ACCESS should succeed";
    
    const auto& access = result.value();
    std::cout << "ACCESS on root:" << std::endl;
    std::cout << "  access: 0x" << std::hex << access.access << std::dec << std::endl;
}

TEST_F(RealRpcTest, ReadDirRoot) {
    auto endpoint = nfs3::RPCEndpoint::create(server_host, 2049);
    ASSERT_TRUE(endpoint.is_connected());
    
    nfs3::NFS3TestClient client(std::move(endpoint));
    
    auto result = client.readdir(root(), 0, 0, 8192);
    ASSERT_TRUE(result.has_value()) << "READDIR should succeed";
    
    const auto& dir = result.value();
    std::cout << "READDIR entries:" << std::endl;
    
    nfs3::entry3* entry = dir.reply.entries.get();
    while (entry != nullptr) {
        std::cout << "  " << entry->name << " (cookie=" << entry->cookie << ")" << std::endl;
        entry = entry->nextentry.get();
    }
    std::cout << "  eof=" << dir.reply.eof << std::endl;
}

TEST_F(RealRpcTest, CreateWriteReadRemove) {
    auto endpoint = nfs3::RPCEndpoint::create(server_host, 2049);
    ASSERT_TRUE(endpoint.is_connected());
    
    nfs3::NFS3TestClient client(std::move(endpoint));
    
    std::string test_filename = "test_rpc_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    
    nfs3::sattr3 attrs;
    attrs.mode = 0644;
    attrs.uid = 1000;
    attrs.gid = 1000;
    attrs.size = 0;
    
    auto create_result = client.create(root(), test_filename, nfs3::createmode3::UNCHECKED, attrs);
    ASSERT_TRUE(create_result.has_value()) << "CREATE should succeed";
    
    const auto& created = create_result.value();
    std::cout << "Created file: " << test_filename << std::endl;
    std::cout << "  handle size: " << created.object.data.size() << std::endl;
    
    std::vector<uint8_t> test_data = {'H', 'e', 'l', 'l', 'o', ' ', 'N', 'F', 'S', '3', '!'};
    
    auto write_result = client.write(created.object, 0, nfs3::stable_how::DATA_SYNC, test_data);
    ASSERT_TRUE(write_result.has_value()) << "WRITE should succeed";
    EXPECT_EQ(write_result->count, test_data.size());
    std::cout << "Wrote " << write_result->count << " bytes" << std::endl;
    
    auto read_result = client.read(created.object, 0, 1024);
    ASSERT_TRUE(read_result.has_value()) << "READ should succeed";
    EXPECT_EQ(read_result->data, test_data);
    std::cout << "Read " << read_result->data.size() << " bytes, verified content matches" << std::endl;
    
    auto remove_result = client.remove(root(), test_filename);
    ASSERT_TRUE(remove_result.has_value()) << "REMOVE should succeed";
    std::cout << "Removed file: " << test_filename << std::endl;
}

TEST_F(RealRpcTest, MkdirRmdir) {
    auto endpoint = nfs3::RPCEndpoint::create(server_host, 2049);
    ASSERT_TRUE(endpoint.is_connected());
    
    nfs3::NFS3TestClient client(std::move(endpoint));
    
    std::string test_dirname = "test_dir_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    
    nfs3::sattr3 attrs;
    attrs.mode = 0755;
    attrs.uid = 1000;
    attrs.gid = 1000;
    
    auto mkdir_result = client.mkdir(root(), test_dirname, attrs);
    ASSERT_TRUE(mkdir_result.has_value()) << "MKDIR should succeed";
    std::cout << "Created directory: " << test_dirname << std::endl;
    
    auto getattr_result = client.getattr(mkdir_result->object);
    ASSERT_TRUE(getattr_result.has_value());
    EXPECT_EQ(getattr_result->obj_attributes.type_, nfs3::ftype3::NF3DIR);
    std::cout << "Verified directory type is NF3DIR" << std::endl;
    
    auto rmdir_result = client.rmdir(root(), test_dirname);
    ASSERT_TRUE(rmdir_result.has_value()) << "RMDIR should succeed";
    std::cout << "Removed directory: " << test_dirname << std::endl;
}

TEST_F(RealRpcTest, LookupNonExistent) {
    auto endpoint = nfs3::RPCEndpoint::create(server_host, 2049);
    ASSERT_TRUE(endpoint.is_connected());
    
    nfs3::NFS3TestClient client(std::move(endpoint));
    
    auto result = client.lookup(root(), "nonexistent_file_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    EXPECT_FALSE(result.has_value()) << "LOOKUP on non-existent file should fail";
    std::cout << "LOOKUP correctly returned error for non-existent file" << std::endl;
}

TEST_F(RealRpcTest, GetAttrInvalidHandle) {
    auto endpoint = nfs3::RPCEndpoint::create(server_host, 2049);
    ASSERT_TRUE(endpoint.is_connected());
    
    nfs3::NFS3TestClient client(std::move(endpoint));
    
    nfs3::nfs_fh3 invalid_fh;
    invalid_fh.data.resize(64, 0);
    
    auto result = client.getattr(invalid_fh);
    EXPECT_FALSE(result.has_value()) << "GETATTR with invalid handle should fail";
    std::cout << "GETATTR correctly returned error for invalid handle" << std::endl;
}
