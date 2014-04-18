#ifndef __SEMANTIC_H__
#define __SEMANTIC_H__

#define MAX_VARIABLE 16384
#define NAME_LEN 16

typedef struct Type_ Type;
typedef struct FieldList_ FieldList;

struct Type_ {
  enum {basic, array, structure} kind;
  union {
    int basic;
    struct {
      Type* elem;
      int size;
    } array;
    FieldList* structure;
  } u;
};

struct FieldList_ {
  char name[NAME_LEN];
  Type *type;
  FieldList *tail;
};

extern FieldList* varlist[MAX_VARIABLE];
#endif