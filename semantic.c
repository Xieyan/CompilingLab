#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "tree.h"
#include "semantic.h"

FieldList* varlist[MAX_VARIABLE] = {NULL};
Type * typelist[MAX_VARIABLE] = {NULL};

Type vartype;
Type * typeptr = NULL;

unsigned int hash_pjw(char *name){
  unsigned int val = 0, i;
  for (; *name ; ++ name){
    val = (val << 2) + *name;
    if (i = val & ~0x3fff)
      val = (val ^ (i>>12)) & 0x3fff;
  }
  return val;
}

void InitialHashTable(){
  int i=0;
  for (i=0 ; i<MAX_VARIABLE; i++){
    varlist[i] = NULL ;	
  }
}

void InitialFieldList(FieldList * variable){
  variable->type = NULL;
  variable->tail = NULL;
}

int addType(Type* type){
  char *name = type->u.structure.name;
  unsigned int probe = hash_pjw(name);
  while (typelist[probe] != NULL){
    char *Hashname = typelist[probe]->u.structure.name;		
    if ( strcmp(name , Hashname) != 0 ) probe++;
    else return -1; 	
  }
  typelist[probe] = type;

  printf("probe : = %d\n" , probe);
  printf("typelist[].kind : = %d\n" , typelist[probe]->kind);
  printf("typelist[].name : = %s\n" , typelist[probe]->u.structure.name);
  printf("typelist[].type : = %d\n" , typelist[probe]->u.structure.structure);
}

int addVar(FieldList* variable){
  char *name = variable->name;
  unsigned int probe = hash_pjw(name);
  while (varlist[probe] != NULL){
    char *Hashname = varlist[probe]->name;		
    if ( strcmp(name , Hashname) != 0 ) probe++;
    else return -1; 	
  }
  varlist[probe] = variable;

  printf("probe : = %d\n" , probe);
  printf("hash[].name : = %s\n" , varlist[probe]->name);
  printf("hash[].kind : = %d\n" , varlist[probe]->type->kind);
  printf("hash[].type : = %d\n" , varlist[probe]->type->u.basic);
}

int findVar(char *name){
  unsigned int probe = hash_pjw(name);
  while (varlist[probe] != NULL){
    char *Hashname = varlist[probe]->name;		
    if ( strcmp(name , Hashname) == 0 ) return probe;
    else probe++;
  }
  return -1;	
}

int subtreeDef(node * p, Type * upperlevel, FieldList * fieldspace) {
  bool isStruct = false;
  //FieldList * fl = (FieldList *) malloc(sizeof(FieldList));
  vartype.kind = basic;
  vartype.u.basic = 0;
  //fl->type = type;
  assert(p->label == NODE_NONTERMINATE);

  p = p->child;
  //Specifier
  switch (p->child->label) {
    case NODE_TYPE:
      vartype.kind = basic;
      vartype.u.basic = p->child->nvalue.value_type;
      //addDec(p->sibling, Type type, NULL);
      break;
    case NODE_NONTERMINATE:
      assert(p->child->ntype.type_nonterm == StructSpecifier);
      subtreeStructSpecifier(p->child, NULL);
      isStruct = true;
      //vartype.kind = structure;
      //strncpy(vartype.u.structure.name, p->child->sibling->child->nvalue.value_id, MAXID);
      break;
    default:
      printf("error in Def\n");
      break;
  }

  p = p->sibling;
  //Declist
  printf("start add DecList\n");
  subtreeDecList(p, upperlevel, isStruct);
  printf("end add DecList\n");
  //SEMI
}

int subtreeExtDef(node * p, Type * upperlevel, FieldList * fieldspace) {
  bool isStruct = false;
  vartype.kind = basic;
  vartype.u.basic = 0;
  //fl->type = type;
  assert(p->label == NODE_NONTERMINATE);

  p = p->child;
  //Specifier
  switch (p->child->label) {
    case NODE_TYPE:
      vartype.kind = basic;
      vartype.u.basic = p->child->nvalue.value_type;
      //addDec(p->sibling, Type type, NULL);
      break;
    case NODE_NONTERMINATE:
      assert(p->child->ntype.type_nonterm == StructSpecifier);
      subtreeStructSpecifier(p->child, upperlevel, NULL);
      isStruct = true;
      //memcpy(&vartype,  = structure;
      //strncpy(vartype.u.structure.name, p->child->sibling->child->nvalue.value_id, MAXID);
      break;
    default:
      printf("error in Def\n");
      break;
  }

  p = p->sibling;
  //Declist
  printf("start add DecList\n");
  subtreeDecList(p, upperlevel, isStruct);
  printf("end add DecList\n");
  //SEMI
  return 0;
}

int subtreeStructSpecifier(node * p, Type * upperlevel, FieldList * fieldspace) {
  if (p != NULL && p->child->sibling->sibling->sibling != NULL) {
    node * q = p;
    //StructSpecifier define
    p = p->child;
    assert(p->label == NODE_TERMINATE && p->ntype.type_term == eSTRUCT);
    if (p->sibling->label != NODE_NONTERMINATE) {
      // anonimous struct
      printf("anonimous structure not support currently in beta version\n");      
    } else {
      Type * type = (Type *) malloc(sizeof(Type));
      type->kind = structure;
      strncpy(type->u.structure.name, p->sibling->child->nvalue.value_id, MAXID);
      addType(type);
      typeptr = type;
      semantic(q->child->sibling->sibling->sibling, type); 
    }
  }
}

int subtreeDecList(node * p, Type * upperlevel, bool isStruct) {
  if (p != NULL) {
    node * subtree = p;
    if (p->label == NODE_NONTERMINATE && p->ntype.type_nonterm == VarDec
        && p->child->label == NODE_ID) {

      FieldList * fl = (FieldList *) malloc(sizeof(FieldList));
      if (!isStruct) {
        //basic type
        fl->type = (Type *) malloc(sizeof(Type));
        memcpy((void *)fl->type, (void *)&vartype, sizeof(Type));
        strncpy(fl->name, p->child->nvalue.value_id, MAXID);
        fl->tail = NULL;
      } else {
        //basic structrue
        fl->type = typeptr;
        strncpy(fl->name, p->child->nvalue.value_id, MAXID);
        fl->tail = NULL;
      }

      //array
      while (p->parent->label == NODE_NONTERMINATE && p->parent->ntype.type_nonterm == VarDec) {
        p = p->parent;
        Type * type = (Type *) malloc(sizeof(Type)), * temp = fl->type;
        type->kind = array;
        type->u.array.size = p->child->sibling->sibling->nvalue.value_int;

        if (fl->type->kind == basic) {
          type->u.array.elem = fl->type;
          fl->type = type;
        } else {
          while (temp->u.array.elem->kind == array) {
            temp = temp->u.array.elem;
          }
          type->u.array.elem = temp->u.array.elem;
          temp->u.array.elem = type;
        }
      }

      //add structure using space
      if (upperlevel != NULL) {
        Type * upper = upperlevel;
        FieldList * ahead = NULL;
        if (upper->u.structure.structure == NULL) {
          upper->u.structure.structure = fl;
        } else {
          ahead = upper->u.structure.structure;
          while (true) {
            if (ahead->tail == NULL) {
              ahead->tail = fl;
              break;
            }
            ahead = ahead->tail;
          }
        }
      }


      addVar(fl);
    }
    subtreeDecList(subtree->child, upperlevel, isStruct);
    subtreeDecList(subtree->sibling, upperlevel, isStruct);
  }
}

void semantic(node * p, Type * upperlevel) {
  if (p != NULL) {
    if (p->label == NODE_NONTERMINATE ) {
      printf("%s (%d)\n", stringNonTerminate[p->ntype.type_nonterm], p->lineno);
      switch (p->ntype.type_nonterm) {
        case Def:
          printf("start add Def\n");
          subtreeDef(p, upperlevel, NULL);
          printf("end add Def\n");
          semantic(p->sibling, upperlevel);
          break;
        case ExtDef:
          printf("start add ExtDef\n");
          subtreeDef(p, upperlevel, NULL);
          printf("end add ExtDef\n");
          semantic(p->sibling, upperlevel);
          break;
        default:
          semantic(p->child, upperlevel);
          semantic(p->sibling, upperlevel);
          break;
      }
    } else {
      semantic(p->child, upperlevel);
      semantic(p->sibling, upperlevel);
    }
  }
}

void getvarlist() {
  int i = 0;
  Type * type = NULL;
  for (i = 0; i < MAX_VARIABLE; i ++) {
    if (varlist[i] != NULL ) {
      printf("varlist[%d]: %s\n", i, varlist[i]->name);
      switch (varlist[i]->type->kind) {
        case basic :
          printf("\tkind: basic\n");
          printf("\tu: %s\n", (varlist[i]->type->u.basic == eINTTYPE) ? "INT" : "FLOAT");
          break;
        case array :
          printf("\tkind: array\n");
          printf("\tu: ");
          type = varlist[i]->type;
          while (type->kind == array) {
            printf("%d ", type->u.array.size);
            type = type->u.array.elem;
          }
          if (type->kind == basic) {
            printf("%s \n", (type->u.basic == eINTTYPE) ? "INT" : "FLOAT");
          } else {
            printf("%s \n", type->u.structure.name);
          }
          break;
        case structure :
          printf("\tkind: structure\n");
          printf("\tu: struct %s\n", varlist[i]->type->u.structure.name);
          break;
        default :
          printf("\tvariable invalid\n");
          break;
      }
    }
  }
}


void gettypelist() {
  int i = 0;
  Type * type = NULL;
  FieldList * member = NULL;
  for (i = 0; i < MAX_VARIABLE; i ++) {
    if (typelist[i] != NULL ) {
      printf("typelist[%d]: struct %s\n", i, typelist[i]->u.structure.name);
      member = typelist[i]->u.structure.structure;
      while (member != NULL) {
        switch (member->type->kind) {
          case basic :
            printf("\tmember: %s\n", member->name);
            printf("\tu: %s\n", (member->type->u.basic == eINTTYPE) ? "INT" : "FLOAT");
            break;
          case array :
            printf("\tmember: %s\n", member->name);
            printf("\tu: ");
            type = member->type;
            while (type->kind == array) {
              printf("%d ", type->u.array.size);
              type = type->u.array.elem;
            }
            if (type->kind == basic) {
              printf("%s \n", (type->u.basic == eINTTYPE) ? "INT" : "FLOAT");
            } else {
              printf("%s \n", type->u.structure.name);
            }
            break;
          case structure :
            printf("\tmember: %s\n", member->name);
            printf("\tu: struct %s\n", member->type->u.structure.name);
            break;
          default :
            printf("\ttype invalid\n");
            break;
        }
        member = member->tail;
      }
    }
  }
}
