

#include "restream.h"
#include "guide.h"
#include "playlist.h"
#include "infile.h"
#include "reader.h"
#include "writer.h"


void *reader(void *parms)
{
    /* This function runs on its own thread and monitors the pipe */

    int pipefd_r;
    char *byte_buffer[4096];
    int reader_active;
    ssize_t bytes_read;
    ctx_restream *restrm = parms;

    pthread_setname_np(pthread_self(), "OutputReader");

    snprintf(restrm->function_name,1024,"%s","reader");

    //fprintf(stderr, "%s: reader started %d\n"
    //    ,restrm->guide_info->guide_displayname,thread_count);

    pipefd_r = 0;
    reader_active = TRUE;
    restrm->reader_action=READER_ACTION_CLOSE;

    while (reader_active){
        switch(restrm->reader_action) {
        case READER_ACTION_START:
            if (pipefd_r == 0) pipefd_r = open(restrm->out_filename, O_RDONLY | O_NONBLOCK);
            if (pipefd_r == -1 ) {
                pipefd_r = 0;
                restrm->reader_status = READER_STATUS_CLOSED;
            } else {
                restrm->reader_status = READER_STATUS_READING;
            }
            break;
        case READER_ACTION_BYTE:
            if (pipefd_r == 0) pipefd_r = open(restrm->out_filename, O_RDONLY | O_NONBLOCK);
            if (pipefd_r == -1 ) {
                pipefd_r = 0;
                restrm->reader_status = READER_STATUS_CLOSED;
            } else {
                restrm->reader_status = READER_STATUS_READBYTE;
            }
            break;
        case READER_ACTION_OPEN:
            if (pipefd_r == 0){
                pipefd_r = open(restrm->out_filename, O_RDONLY | O_NONBLOCK);
                if (pipefd_r == -1 ) {
                    pipefd_r = 0;
                    restrm->reader_status = READER_STATUS_CLOSED;
                } else {
                    restrm->reader_status = READER_STATUS_PAUSED;
                }
            }
            break;
        case READER_ACTION_CLOSE:
            if (pipefd_r != 0){
                close(pipefd_r);
                pipefd_r = 0;
            }
            restrm->reader_status = READER_STATUS_CLOSED;
            break;
        case READER_ACTION_END:
            if (pipefd_r != 0){
                close(pipefd_r);
                pipefd_r = 0;
                restrm->reader_status = READER_STATUS_CLOSED;
            }
            reader_active = FALSE;
            break;
        }

        if ((restrm->reader_status == READER_STATUS_READING) && (pipefd_r != 0) ){
            bytes_read=read(pipefd_r, byte_buffer, sizeof(byte_buffer));
            (void)bytes_read;
        } else if ((restrm->reader_status == READER_STATUS_READBYTE) && (pipefd_r != 0) ){
            bytes_read=read(pipefd_r, byte_buffer, sizeof(byte_buffer));
            (void)bytes_read;
            SLEEP(0, 10000000);
        } else {
            SLEEP(1,0);
        }

        restrm->watchdog_reader=av_gettime_relative();
    }

    //fprintf(stderr, "%s: reader exit %d\n"
    //    ,restrm->guide_info->guide_displayname,thread_count);

    restrm->reader_status = READER_STATUS_INACTIVE;

    pthread_exit(NULL);
}

void reader_start(ctx_restream *restrm)
{
    /*
    fprintf(stderr,"%s: reader_start enter %d \n"
        ,restrm->guide_info->guide_displayname
        ,restrm->reader_status);
    */

    if (restrm->reader_status != READER_STATUS_READING){
        pthread_mutex_lock(&restrm->mutex_reader);
            restrm->reader_action = READER_ACTION_START;
            while (restrm->reader_status != READER_STATUS_READING);
        pthread_mutex_unlock(&restrm->mutex_reader);
    }

    /* 
    fprintf(stderr,"%s: reader_start exit %d \n"
        ,restrm->guide_info->guide_displayname
        ,restrm->reader_status);
    */
}

void reader_startbyte(ctx_restream *restrm) 
{
    /*
    fprintf(stderr,"%s: reader_startbyte enter %d \n"
        ,restrm->guide_info->guide_displayname
        ,restrm->reader_status);
    */

    if (restrm->reader_status != READER_STATUS_READBYTE){
        pthread_mutex_lock(&restrm->mutex_reader);
            restrm->reader_action = READER_ACTION_BYTE;
            while (restrm->reader_status != READER_STATUS_READBYTE);
        pthread_mutex_unlock(&restrm->mutex_reader);
    }

    /*
    fprintf(stderr,"%s: reader_startbyte exit %d \n"
        ,restrm->guide_info->guide_displayname
        ,restrm->reader_status);
    */
}

void reader_close(ctx_restream *restrm)
{
    /*
    fprintf(stderr,"%s: reader_close enter %d \n"
        ,restrm->guide_info->guide_displayname
        ,restrm->reader_status);
    */

    pthread_mutex_lock(&restrm->mutex_reader);
        restrm->reader_action = READER_ACTION_CLOSE;
        while (restrm->reader_status != READER_STATUS_CLOSED);
    pthread_mutex_unlock(&restrm->mutex_reader);

    /*
    fprintf(stderr,"%s: reader_close exit %d \n"
        ,restrm->guide_info->guide_displayname
        ,restrm->reader_status);
    */
}

void reader_end(ctx_restream *restrm)
{
    /*
    fprintf(stderr,"%s: reader_end enter %d \n"
        ,restrm->guide_info->guide_displayname
        ,restrm->reader_status);
    */

    pthread_mutex_lock(&restrm->mutex_reader);
        restrm->reader_action = READER_ACTION_END;
        while (restrm->reader_status != READER_STATUS_INACTIVE);
    pthread_mutex_unlock(&restrm->mutex_reader);

    pthread_mutex_destroy(&restrm->mutex_reader);

    /*
    fprintf(stderr,"%s: reader_end exit %d \n"
        ,restrm->guide_info->guide_displayname
        ,restrm->reader_status);
    */
}

void reader_flush(ctx_restream *restrm)
{
    /*
    fprintf(stderr,"%s: reader_flush enter %d \n"
        ,restrm->guide_info->guide_displayname
        ,restrm->reader_status);
    */

    snprintf(restrm->function_name,1024,"%s","reader_flush");

    reader_start(restrm);

    sleep(1);

    reader_close(restrm);

    /*
    fprintf(stderr,"%s: reader_flush exit %d \n"
        ,restrm->guide_info->guide_displayname
        ,restrm->reader_status);
    */
}

void reader_init(ctx_restream *restrm)
{

    snprintf(restrm->function_name,1024,"%s","reader_init");

    /*
    fprintf(stderr,"%s: reader_init enter %d \n"
        ,restrm->guide_info->guide_displayname
        ,restrm->reader_status);
    */

    pthread_mutex_init(&restrm->mutex_reader, NULL);

    pthread_attr_t handler_attribute;

    restrm->watchdog_reader=av_gettime_relative();

    pthread_attr_init(&handler_attribute);
    pthread_attr_setdetachstate(&handler_attribute, PTHREAD_CREATE_DETACHED);

    pthread_create(&restrm->reader_thread, &handler_attribute, reader, restrm);

    pthread_attr_destroy(&handler_attribute);

    reader_close(restrm);

    /*
    fprintf(stderr,"%s: reader_init exit %d \n"
        ,restrm->guide_info->guide_displayname
        ,restrm->reader_status);
    */
}
