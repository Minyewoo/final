#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <string.h>

char* validate_dirpath(char* dirpath)
{
    char *validated_path;
    int length = strlen(dirpath);

    if(dirpath[length-1]!='/')
    {
        validated_path = (char*)malloc(length+2);
        strcpy(validated_path, dirpath);
        strcat(validated_path,"/");
        return validated_path;
    }
    else
        return dirpath;
}

struct Options {
    const char* ip_addr;
    int port;
    const char* root_dir;

    Options(int argc, char **argv)
    {
        int opt;

        while((opt = getopt(argc, argv, "h:p:d:")) != -1)
        {
            switch(opt)
            {
                case 'h':
                    ip_addr = optarg;
                    printf("host ip address: %s\n", ip_addr);
                    break;
                case 'p':
                    port =  atoi(optarg);
                    printf("port: %d\n", port);
                    break;
                case 'd':
                    root_dir = validate_dirpath(optarg);
                    printf("root directory: %s\n", root_dir);
                    break;
            }
        }
    }
};

char* read_all_file(char *path)
{
    char *file_content = NULL;

    FILE *file = fopen(path,"r");
    if(file)
    {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file,0,SEEK_SET);

        file_content = (char*)malloc(size);
        fread(file_content, 1, size, file);

        fclose(file);

        return file_content;
    }
    else
        return NULL;
}

char* get_path_from_request(char *buf)
{
    int len = strlen(buf);
    char *copy = (char*)malloc(len+1);
    strcpy(copy, buf);

    char *path;

    if(strtok(copy, " "))
    {
        path = strtok(NULL, " ");

        if(path) {
            path = strdup(path);
        }
    }

    free(copy);
    return path;
}

char* validate_filepath(char* filepath)
{
    char *validated_path;
    char *index_file="index.html";

    char* question_char_ptr = strstr(filepath,"?");
    if(question_char_ptr)
        *question_char_ptr='\0';

    int length = strlen(filepath);

    if(filepath[0]=='/' && length > 1) {
        validated_path = (char*)malloc(length+strlen(index_file));
        strcpy(validated_path, filepath+1);
    }
    else if(length > 1)
    {
        validated_path = (char*)malloc(length+strlen(index_file)+1);
        strcpy(validated_path, filepath);
    }

    if(filepath[length - 1] == '/') {
        if(!validated_path)
            validated_path = (char*)malloc(strlen(index_file)+1);

        strcat(validated_path, index_file);
    }
    return validated_path;
}

const char* ok_response_template = "HTTP/1.0 200 OK\r\n"

                                   "Content-length: %d\r\n"

                                   "Content-Type: text/html\r\n"

                                   "\r\n"

                                   "%s";

const char* failure_response_template = "HTTP/1.0 404 NOT FOUND\r\n"
                                        "Content-Type: text/html\r\n"
                                        "\r\n";

const char* directory;
uv_loop_t *loop;

void read_callback(uv_stream_t *stream, ssize_t nread, uv_buf_t buf)
{
    uv_buf_t response_buf;
    char *response_text;

    char* filepath = validate_filepath(get_path_from_request(buf.base));
    char* full_path = (char*)malloc(strlen(filepath) + strlen(directory) + 1);
    strcpy(full_path, directory);
    strcat(full_path, filepath);

    if(access( full_path, F_OK ) != -1)
    {
        char *file_content = read_all_file(full_path);
        response_text = (char*)malloc(strlen(ok_response_template)+strlen(file_content)+11);
        sprintf(response_text, ok_response_template, strlen(file_content), file_content);


        free(file_content);
    }
    else
    {
        response_text = (char*)malloc(strlen(failure_response_template)+1);
        strcpy(response_text, failure_response_template);
    }

    response_buf = uv_buf_init(response_text, strlen(response_text));
    uv_write_t *response = (uv_write_t*)malloc(sizeof(uv_write_t));
    uv_write(response, stream, &response_buf, 1, NULL);

    free(response_text);
    free(buf.base);
    free((void *)filepath);
    free(full_path);

    uv_close((uv_handle_t*)stream, NULL);
}

uv_buf_t alloc_buffer(uv_handle_t *handle, size_t size)
{
    return uv_buf_init((char*)malloc(size), size);
}

void connection_callback(uv_stream_t *server, int status)
{
    uv_tcp_t *client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);
    uv_accept(server, (uv_stream_t*)client);
    uv_read_start((uv_stream_t *) client, alloc_buffer, read_callback);
}

int main(int argc, char **argv)
{
    Options options(argc, argv);

    int thread = 0;
    if(!options.ip_addr || !options.port || !options.root_dir)
    {
        printf("%s","Wrong arguments!");
        return thread;
    }

    directory = options.root_dir;
    uv_tcp_t server;

    loop = uv_default_loop();

    struct sockaddr_in addr = uv_ip4_addr(options.ip_addr,options.port);

    uv_tcp_init(loop, &server);
    uv_tcp_bind(&server, addr);
    uv_listen((uv_stream_t *) &server, 128, connection_callback);

    if(!fork())
        return uv_run(loop, UV_RUN_DEFAULT);
    else
        return 0;
}


//thread t(uv_read_start,(uv_stream_t *) client, alloc_buffer, read_callback);
