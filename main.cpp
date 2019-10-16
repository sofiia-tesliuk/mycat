#include <iostream>
#include <boost/program_options.hpp>
#include <vector>
#include <fcntl.h>
#include <iomanip>


class IOManager{
public:
    int write_buffer(int fd, const char* buffer, ssize_t size, int* status){
        ssize_t written_bytes = 0;
        while (written_bytes < size){
            ssize_t written_now = write(fd, buffer + written_bytes, size - written_bytes);
            if (written_now == -1){
                if (errno == EINTR)
                    continue;
                else{
                    *status = errno;
                    return EXIT_FAILURE;
                }
            } else
                written_bytes += written_now;
        }
        return EXIT_SUCCESS;
    }

    int read_buffer(int fd, char* buffer, ssize_t size, int* status){
        ssize_t read_bytes = 0;
        while (read_bytes < size){
            ssize_t read_now = read(fd, buffer + read_bytes, size - read_bytes);
            if (read_now == -1){
                if (errno == EINTR)
                    continue;
                else{
                    *status = errno;
                    return EXIT_FAILURE;
                }
            } else
                read_bytes += read_now;
        }
        return EXIT_SUCCESS;
    }
};

class MyCat{
public:
    int run(int &argc, const char *argv[]){
        if (parse_arguments(argc, argv) != EXIT_SUCCESS)
            return EXIT_FAILURE;

        if (mode == HELP_MODE){
            std::cout << desc << '\n';
            return EXIT_SUCCESS;
        }

        std::vector<int> file_descriptors;
        int fd;

        // Opening files
        for (int i = 1; i < argc; i++){
            if (argv[i][0] != '-'){
                fd = open(argv[i], O_RDONLY);
                std::cout << fd << std::endl;
                if (fd == -1){
                    std::cerr << "Failed to open " << argv[i] << std::endl;
                    return EXIT_FAILURE;
                } else
                    file_descriptors.push_back(fd);
            }
        }

        if (mode == NORMAL_MODE)
            return read_and_write(file_descriptors);
        else
            return read_and_write_with_hidden_chars(file_descriptors);
    }

private:
    boost::program_options::options_description desc{"Options"};
    enum { HELP_MODE, NORMAL_MODE, PRINT_HIDDEN_SYMBOLS_MODE } mode = NORMAL_MODE;

    int parse_arguments(int &argc, const char *argv[]){
        try
        {
            desc.add_options()
                    ("help,h", "Help screen")
                    ("A,A", "Print with hidden symbols");

            boost::program_options::variables_map vm;
            store(boost::program_options::parse_command_line(argc, argv, desc), vm);
            notify(vm);

            if (vm.count("help"))
                mode = HELP_MODE;
            else if (vm.count("A"))
                mode = PRINT_HIDDEN_SYMBOLS_MODE;
        }
        catch (const boost::program_options::error &ex)
        {
            std::cerr << ex.what() << '\n';
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    int read_and_write(std::vector<int> &file_descriptors){
        IOManager io;
        size_t buffer_size = 4096u;
        char* buffer = (char*) malloc(buffer_size);

        int* status = 0;
        ssize_t fsize;
        ssize_t read_size = 0u;

        if (buffer == nullptr){
            std::cerr << "Unable to allocate the buffer" << std::endl;
            return EXIT_FAILURE;
        }

        // Reading content of files and write to stdout
        for(auto const& fd: file_descriptors) {
            // Get size of file
            fsize = lseek(fd, (size_t)0, SEEK_END);
            // Place pointer to the beginning if the file
            lseek(fd, (size_t)0, SEEK_SET);
            // Haven't read whole file
            while(fsize > 0u){
                read_size = fsize < buffer_size ? fsize : buffer_size;
                if (io.read_buffer(fd, buffer, read_size, status) == EXIT_FAILURE){
                    std::cerr << "Failed to read file with fd: " << fd << std::endl;
                    std::cerr << "Status: " << status << std::endl;
                    return EXIT_FAILURE;
                }

                if (io.write_buffer(STDOUT_FILENO, buffer, read_size, status) == EXIT_FAILURE){
                    std::cerr << "Failed to write file with fd: " << fd << std::endl;
                    std::cerr << "Status: " << status << std::endl;
                    return EXIT_FAILURE;
                }

                fsize -= read_size;
            }

            if (close(fd) == -1){
                std::cerr << "Failed to close file with fd: " << fd << std::endl;
            }
        }

        std::free(buffer);
        return EXIT_SUCCESS;
    }

    int read_and_write_with_hidden_chars(std::vector<int> &file_descriptors){
        IOManager io;
        size_t buffer_size = 1024u;
        char* buffer = (char*) malloc(buffer_size);
        char* processed_buffer = (char*) malloc(4 * buffer_size);

        int* status = 0;
        ssize_t fsize;
        ssize_t read_size = 0u;
        ssize_t write_size = 0u;

        if ((buffer == nullptr) || (processed_buffer == nullptr)){
            std::cerr << "Unable to allocate buffers" << std::endl;
            return EXIT_FAILURE;
        }

        // Reading content of files and write to stdout
        for(auto const& fd: file_descriptors) {
            fsize = lseek(fd, (size_t)0, SEEK_END);
            lseek(fd, (size_t)0, SEEK_SET);
            while(fsize > 0u){
                read_size = fsize < buffer_size ? fsize : buffer_size;
                if (io.read_buffer(fd, buffer, read_size, status) == EXIT_FAILURE){
                    std::cerr << "Failed to read file with fd: " << fd << std::endl;
                    std::cerr << "Status: " << status << std::endl;
                    return EXIT_FAILURE;
                }

                write_size = 0u;

                // Processing each char
                for (int i = 0; i < read_size; i++){
                    if ((isprint(buffer[i])) || (isspace(buffer[i]))){
                        processed_buffer[write_size] = buffer[i];
                        write_size++;
                    } else {
                        std::string processed_char = int_to_hex(buffer[i]);
                        for (int j = 0; j < 4; j++){
                            processed_buffer[write_size + j] = processed_char[j];
                        }
                        write_size += 4;
                    }
                }

                if (io.write_buffer(STDOUT_FILENO, processed_buffer, write_size, status) == EXIT_FAILURE){
                    std::cerr << "Failed to write file with fd: " << fd << std::endl;
                    std::cerr << "Status: " << status << std::endl;
                    return EXIT_FAILURE;
                }

                fsize -= read_size;
            }

            if (close(fd) == -1){
                std::cerr << "Failed to close file with fd: " << fd << std::endl;
            }
        }

        std::free(buffer);
        return EXIT_SUCCESS;
    }

    std::string int_to_hex(int i){
        std::stringstream stream;
        stream << "\\x"
               << std::setfill ('0') << std::setw(sizeof(int)*2)
               << std::hex << i;
        return stream.str();
    }
};


int main(int argc, const char *argv[]) {
    MyCat kitty;

    if (kitty.run(argc, argv) != EXIT_SUCCESS)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}