//
// Created by Chanchan on 11/9/21.
//

#include "HttpServer.h"
#include "HttpMsg.h"
#include "workflow/WFFacilities.h"
#include "PathUtil.h"
#include <csignal>

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;
    svr.mount("/static");

    // An expriment (Upload a file to parent dir is really dangerous.):
    // curl -v -X POST "ip:port/upload" -F "file=@demo.txt; filename=../demo.txt" -H "Content-Type: multipart/form-data"
    // Then you find the file is store in the parent dir, which is dangerous
    svr.Post("/upload", [](HttpReq *req, HttpResp *resp)
    {
        auto files = req->post_files();
        if(files.empty())
        {
            resp->set_status(HttpStatusBadRequest);
        } else
        {
            auto *file = files[0];
            // file->filename SHOULD NOT be trusted. See Content-Disposition on MDN
            // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition#directives
            // The filename is always optional and must not be used blindly by the application:
            // path information should be stripped, and conversion to the server file system rules should be done.
            fprintf(stderr, "filename : %s\n", file->filename.c_str());
            resp->Save(file->filename, std::move(file->content));
        }
    });

    svr.Post("/upload_fix", [](HttpReq *req, HttpResp *resp)
    {
        auto files = req->post_files();
        if(files.empty())
        {
            resp->set_status(HttpStatusBadRequest);
        } else
        {
            auto *file = files[0];
            // simple solution to fix the problem above
            // This will restrict the upload file to current directory.
            resp->Save(PathUtil::base(file->filename), std::move(file->content));
        }
    });

    // upload multiple files
    // curl -X POST http://ip:port/upload_multiple \
    // -F "file1=@file1" \
    // -F "file2=@file2" \
    // -H "Content-Type: multipart/form-data"
    svr.Post("/upload_multiple", [](HttpReq *req, HttpResp *resp)
    {
        auto files = req->post_files();
        if(files.empty())
        {
            resp->set_status(HttpStatusBadRequest);
        } else
        {
            for(auto& file : files)
            {
                resp->Save(file->filename, std::move(file->content));
            }
        }
    });


    if (svr.start(9001) == 0)
    {
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}