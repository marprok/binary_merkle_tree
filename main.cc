#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <openssl/sha.h>
#include <queue>
#include <sstream>
#include <string>

// Compute the root of the binary merkle tree for a given file.
// Author: Marios Prokopakis

namespace fs = std::filesystem;

std::string get_hash(const std::uint8_t* bytes, std::size_t size)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    if (!SHA256(bytes, size, hash))
    {
        std::cerr << "Error while hashing!\n";
        return {};
    }
    std::stringstream ss;
    for (std::size_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<std::uint32_t>(hash[i]);
    return ss.str();
}

class MerkleTree
{
public:
    bool make(const std::string& file_name, const std::size_t block_size)
    {
        std::queue<std::string> nodes = get_leaf_nodes(file_name, block_size);
        if (nodes.empty())
            return false;
        // if there is an odd number of leaves, add a dummy node
        if (nodes.size() % 2)
            nodes.push("");
        // there can be only one!
        while (nodes.size() > 1)
        {
            auto left = nodes.front();
            nodes.pop();
            auto right = nodes.front();
            nodes.pop();
            const auto hash(left + right);
            nodes.push(get_hash(reinterpret_cast<const std::uint8_t*>(hash.data()), hash.size()));
        }
        m_root = nodes.front();
        return true;
    }

    std::string root_hash() const
    {
        return m_root;
    }

private:
    std::queue<std::string> get_leaf_nodes(const std::string& file_name, std::size_t block_size)
    {
        std::fstream                    in(file_name, std::ios::binary | std::ios::in);
        const std::size_t               file_size    = fs::file_size(file_name);
        std::size_t                     total_blocks = file_size / block_size + !!(file_size % block_size);
        std::queue<std::string>         leaf_nodes;
        std::unique_ptr<std::uint8_t[]> buffer = std::make_unique<std::uint8_t[]>(block_size);

        for (std::size_t blocks_read = 0; blocks_read < total_blocks; ++blocks_read)
        {
            std::streamsize bytes_to_read = block_size;
            if (blocks_read == (total_blocks - 1) && (file_size % block_size))
                bytes_to_read = file_size % block_size;
            in.read(reinterpret_cast<char*>(buffer.get()), bytes_to_read);
            if (in.gcount() != bytes_to_read)
            {
                std::cerr << "get_block_hashes: Could not read the exected number of bytes from the stream!\n";
                return {};
            }
            leaf_nodes.push(get_hash(buffer.get(), bytes_to_read));
        }
        return leaf_nodes;
    }
    std::string m_root;
};

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << "file_name block_size\n";
        std::exit(EXIT_FAILURE);
    }

    if (!fs::exists(argv[1]))
    {
        std::cerr << "File " << argv[1] << " does not exist!\n";
        std::exit(EXIT_FAILURE);
    }

    if (!fs::file_size(argv[1]))
    {
        std::cerr << "File " << argv[1] << " is empty!\n";
        std::exit(EXIT_FAILURE);
    }

    std::size_t block_size = std::stol(argv[2]);
    if (block_size > fs::file_size(argv[1]))
    {
        std::cerr << "Block size is greater than the size of the file!\n";
        std::exit(EXIT_FAILURE);
    }

    MerkleTree tree;
    if (!tree.make(argv[1], std::stol(argv[2])))
        std::exit(EXIT_FAILURE);

    std::cout << "Root: " << tree.root_hash() << std::endl;
    std::exit(EXIT_SUCCESS);
}
