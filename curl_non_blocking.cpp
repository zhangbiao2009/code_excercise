#include <curl/curl.h>
#include <sys/select.h>
#include <unistd.h>
#include <iostream>
using namespace std;

void loop()
{
    CURLM *multi_handle = curl_multi_init();
    int still_running = 0;

    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, "http://www.baidu.com");
    curl_multi_add_handle(multi_handle, curl);

    while(1){
        fd_set fdread;
        fd_set fdwrite;
        fd_set fdexcep;
        int maxfd = -1;

        long curl_timeo;

        curl_multi_timeout(multi_handle, &curl_timeo);
        if(curl_timeo < 0)
            curl_timeo = 1000;

        struct timeval timeout;
        timeout.tv_sec = curl_timeo / 1000;
        timeout.tv_usec = (curl_timeo % 1000) * 1000;
        cout<<"timeout, tv_sec: "<<timeout.tv_sec<<" tv_usec: "<< timeout.tv_usec<<endl;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        /* get file descriptors from the transfers */
        CURLMcode mc = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

        int rc;
        if(maxfd == -1) {
            usleep(100000);
            rc = 0;
        }
        else
            rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

        switch(rc) {
            case -1:
                /* select error */
                break;
            case 0:
            default:
                /* timeout or readable/writable sockets */
                curl_multi_perform(multi_handle, &still_running);
                cout<<"still_running: "<<still_running<<endl;
                if(still_running == 0)  // all finished
                    goto end;
        }
    }
end:
    cout<<"end"<<endl;
    curl_multi_cleanup(multi_handle);
}

int main()
{
    loop();
    return 0;
}
