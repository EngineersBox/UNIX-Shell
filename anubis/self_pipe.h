#ifndef ANUBIS_SELF_PIPE_H
#define ANUBIS_SELF_PIPE_H

int self_pipe_new(int selfPipe[2]);
int self_pipe_send(int selfPipe[2], int signal);
int self_pipe_poll(int selfPipe[2], int* error);
int self_pipe_free(int selfPipe[2]);

#endif // ANUBIS_SELF_PIPE_H
