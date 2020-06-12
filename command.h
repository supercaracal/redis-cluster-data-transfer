#ifndef COMMAND_H_
#define COMMAND_H_

#include "./net.h"

#define MAX_CMD_SIZE (1 << 14)
#define MAX_KEY_SIZE 256
#define DEFAULT_REPLY_LINES 16
#define DEFAULT_REPLY_SIZE 256

#define LAST_LINE(r) (r->i > 0 ? r->lines[r->i - 1] : NULL)
#define LAST_LINE2(r) (r.i > 0 ? r.lines[r.i - 1] : NULL)

#define INIT_REPLY_LINE(r) do {\
  r->lines[r->i] = NULL;\
  r->types[r->i] = UNKNOWN;\
  r->sizes[r->i] = 0;\
  r->nextIdxOfLastLine = 0;\
} while (0)

#define INIT_REPLY(r) do {\
  r->size = DEFAULT_REPLY_LINES;\
  r->i = 0;\
  r->lines = (char **) malloc(sizeof(char *) * r->size);\
  ASSERT_MALLOC(r->lines, "for init reply lines");\
  r->types = (ReplyType *) malloc(sizeof(ReplyType) * r->size);\
  ASSERT_MALLOC(r->types, "for init reply types");\
  r->sizes = (int *) malloc(sizeof(int) * r->size);\
  ASSERT_MALLOC(r->sizes, "for init reply sizes");\
  INIT_REPLY_LINE(r);\
} while (0)

#define EXPAND_REPLY_IF_NEEDED(r) do {\
  if (r->i == r->size) {\
    void *tmp;\
    r->size *= 2;\
    tmp = realloc(r->lines, sizeof(char *) * r->size);\
    ASSERT_REALLOC(tmp, "for reply lines");\
    r->lines = (char **) tmp;\
    tmp = realloc(r->types, sizeof(ReplyType) * r->size);\
    ASSERT_REALLOC(tmp, "for reply types");\
    r->types = (ReplyType *) tmp;\
    tmp = realloc(r->sizes, sizeof(int) * r->size);\
    ASSERT_REALLOC(tmp, "for reply sizes");\
    r->sizes = (int *) tmp;\
  }\
} while (0)

#define ADVANCE_REPLY_LINE(r) do {\
  reply->i++;\
  EXPAND_REPLY_IF_NEEDED(r);\
  INIT_REPLY_LINE(r);\
} while (0)

#define ADD_NULL_REPLY(r) do {\
  r->sizes[r->i] = 0;\
  r->lines[r->i] = NULL;\
  r->types[r->i] = NIL;\
  ADVANCE_REPLY_LINE(r);\
} while (0)

typedef enum { UNKNOWN, STRING, INTEGER, RAW, ERR, NIL, TMPBULKSTR, TMPARR } ReplyType;
typedef struct { int size, i, nextIdxOfLastLine, *sizes; char **lines; ReplyType *types; } Reply;

int command(Conn *, const char *, Reply *);
int pipeline(Conn *, const char *, Reply *, int);
void freeReply(Reply *);
void printReplyLines(const Reply *);

#endif  // COMMAND_H_
