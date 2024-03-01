#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <coroutine>
#include <iostream>
#include <fstream>
#include <memory>
#include <filesystem>

namespace asio = boost::asio;
namespace beast = boost::beast;
using tcp = asio::ip::tcp;

class FileManager : public std::enable_shared_from_this<FileManager> {
public:
    FileManager(asio::io_context& io_context, asio::strand<asio::io_context::executor_type>& strand)
        : io_context_(io_context), strand_(strand) {}
    ~FileManager() {std::printf("Disconnection[]: ...\n");}

    asio::awaitable<void> handle_connection(tcp::socket socket) {
        auto self = shared_from_this();

        try {
            beast::flat_buffer buffer;
            beast::http::request<beast::http::string_body> request;

            co_await beast::http::async_read(socket, buffer, request, asio::use_awaitable);

            switch (request.method()) {
                case beast::http::verb::get:
                    co_spawn(strand_,[&]()->asio::awaitable<void>{
                        co_await self->handle_request(std::move(socket), request.target(),request.target().starts_with("/search"));
                    },asio::detached);
                    break;
                case beast::http::verb::post:
                    co_spawn(strand_,[&]()->asio::awaitable<void>{ // split filename 
                      co_await self->handle_upload(std::move(socket), request.target(), request.body());
                    },asio::detached);
                    break;
                default:
                    beast::http::response<beast::http::string_body> response{beast::http::status::method_not_allowed, request.version()};
                    response.set(beast::http::field::content_type, "text/plain");
                    response.keep_alive(request.keep_alive());
                    response.body() = "405 Method Not Allowed";
                    response.prepare_payload();
                    co_await beast::http::async_write(socket, response, asio::use_awaitable);
                    break;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Exception in connection handler: " << e.what() << std::endl;
        }
    }

private:
    asio::io_context& io_context_;
    asio::strand<asio::io_context::executor_type>& strand_;
    const std::string root="/home/sw/Documents/cxx/res";

    asio::awaitable<void> handle_request(tcp::socket socket, const std::string& path, bool is_search) {
         std::string tem_path=path;
        if(is_search)
            tem_path=path.substr(0,7);
        std::string new_path=root+tem_path;

        std::ifstream file(new_path, std::ios::binary);
        if (!file.is_open()) {
            // File not found
            beast::http::response<beast::http::string_body> response{beast::http::status::not_found, 11};
            response.set(beast::http::field::content_type, "text/plain");
            response.keep_alive(false);
            response.body() = "404 Not Found";
            response.prepare_payload();
            co_await beast::http::async_write(socket, response, asio::use_awaitable);
            co_return;
        }
        // Check if the path is a directory
        if (std::filesystem::is_directory(new_path)) {
            std::vector<std::string> filenames;
            if(is_search){
                    std::string query_string = path.substr(path.find('?') + 1);
                    std::printf("search: %s\n",query_string);
                    int pos=query_string.find("search=");
                    if(pos != std::string::npos){
                             query_string = query_string.substr(pos + 7);
                    }
                    search_files_recursive(root, query_string, filenames);

            }else{
                // get all the filenames upon current directory
                for (const auto& entry : std::filesystem::directory_iterator(new_path)) {
                    filenames.push_back(entry.path().filename().string());
                } 
            }

            std::ostringstream html_body;
            html_body << "<!DOCTYPE html>\n"
                    << "<html>\n"
                    << "<head><title>Files in directory</title></head>\n"
                    << "<body>\n"
                    << "<h1>Files in directory:</h1>\n"
                    << "<form action=\"/search\" method=\"get\">\n"
                    << "<input type=\"text\" name=\"search\" placeholder=\"Search files...\">\n"
                    << "<input type=\"submit\" value=\"Search\">\n"
                    << "</form>\n"
                    << "<ul>\n";
              for (const auto& filename : filenames) {
                std::string filepath = new_path + "/" + filename;
                if (std::filesystem::is_directory(filepath)) {
                    html_body << "<li><button onclick=\"location.href='" << path << filename << "'\">" << filename << "</button></li></br>\n";
                } else {
                    html_body << "<li>" << filename << " - <a href=\"" << path << filename << "\" download>Download</a></li></br>\n";
                }
            }
            html_body << "</ul>\n";
            html_body << "<form action=\"/upload\" method=\"post\" enctype=\"multipart/form-data\">\n"
                    << "<input type=\"file\" name=\"fileToUpload\" id=\"fileToUpload\">\n"
                    << "<input type=\"submit\" value=\"Upload File\" name=\"submit\">\n"
                    << "</form>\n"
                    << "</body>\n"
                    << "</html>";

            beast::http::response<beast::http::string_body> response{beast::http::status::ok, 11};
            response.set(beast::http::field::content_type, "text/html");
            response.keep_alive(false);
            // write html to display all the files below current directory
            response.body()=html_body.str();
            response.prepare_payload();
            co_await beast::http::async_write(socket, response, asio::use_awaitable);
            co_return;
        }
        else{
            // deal with the condition of single file
            beast::http::response<beast::http::file_body> response{beast::http::status::ok, 11};
            response.set(beast::http::field::content_type, "text/plain");
            response.keep_alive(false);
            boost::system::error_code ec;
            // display files below current directory
            response.body().open(new_path.c_str(), beast::file_mode::scan, ec);
            response.prepare_payload();
            co_await beast::http::async_write(socket, response, asio::use_awaitable);
            co_return;
        }
    }

    asio::awaitable<void> handle_upload(tcp::socket socket, const std::string& filename, const std::string& body) {
        try {
            std::ofstream outfile("/home/sw/Documents/cxx/res/upload/" + filename, std::ios::binary);
            outfile.write(body.data(), body.size());
            outfile.close();

            beast::http::response<beast::http::string_body> response{beast::http::status::ok, 11};
            response.set(beast::http::field::content_type, "text/plain");
            response.keep_alive(false);
            response.body() = "File '" + filename + "' uploaded successfully";
            response.prepare_payload();
            co_await beast::http::async_write(socket, response, asio::use_awaitable);
        } catch (const std::exception& e) {
            std::cerr << "Exception during file upload: " << e.what() << std::endl;
        }
    }

    void search_files_recursive(const std::filesystem::path& directory, const std::string& search_keyword, std::vector<std::string>& filenames) {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (std::filesystem::is_directory(entry)) {
                // Recursively search subdirectories
                search_files_recursive(entry, search_keyword, filenames);
            } else if (entry.path().filename().string().find(search_keyword) != std::string::npos) {
                // Check if the filename contains the search keyword
                filenames.push_back(entry.path().filename().string());
            }
    }
}

};
