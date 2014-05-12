#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "tree.h"
#include "semantic.h"
#include "intercode.h"

InterCodes * ichead = NULL;
int temp_count = 0;
int addr_count = 0;
int label_count = 0;

Operand * get_value(int value) {
  Operand * op = (Operand *) malloc(sizeof(Operand));
  op->kind = opCONSTANT;
  op->u.value = value;
  return op;
}

Operand * get_relop(enum RelopValue e) {
  Operand * op = (Operand *) malloc(sizeof(Operand));
  op->kind = opRELOP;
  op->u.relop = e;
  return op;
}

Operand * get_arglist(Operands * arg_list, int index) {
  assert(arg_list != NULL && index >= 0);
  Operands * p = arg_list;
  int i = 0;
  while (p->next != NULL && i < index) {
    p = p->next;
    i ++;
  }
  if (i == index && p != NULL) {
    return &(p->op);
  }
  return NULL;
}

int get_typesize (Type * type) {
  assert(type != NULL);
  int sum = 0;
  FieldList * member = NULL;
  switch (type->kind) {
    case basic:
      sum = 4;
      break;
    case array:
      assert(type->u.array.elem->kind == basic);
      sum = type->u.array.size * 4;
      break;
    case structure:
      member = type->u.structure.structure;
      if (member == NULL) return 0;
      while (member != NULL) {
        sum += get_typesize(member->type);
      }
      break;
    default : break;
  }
  return sum;
}

Operands * insert_arglist_head(Operands * arglist, Operand * arg) {
  assert(arg != NULL);
  Operands * temp = (Operands *) malloc(sizeof(Operands));
  temp->prev = NULL;
  temp->next = NULL;
  memcpy( (void *) &(temp->op), arg, sizeof(Operand) );
  if (arglist == NULL) {
    arglist = temp;
    return arglist;
  } else {
    temp->next = arglist;
    arglist->prev = temp;
    arglist = temp;
    return arglist;
  }
}

Operand * new_temp() {
  Operand * op = (Operand *) malloc(sizeof(Operand));
  op->kind = opTEMP;
  op->u.temp_no = temp_count;
  temp_count ++;
  return op;
}

Operand * new_label() {
  Operand * op = (Operand *) malloc(sizeof(Operand));
  op->kind = opLABEL;
  op->u.label_no = label_count;
  label_count ++;
  return op;
}

Operand * lookup_varlist(char * id) {
  printf("start lookup_varlist\n");
  assert(id != NULL);
  printf("lookup_varlist %s\n", id);
  Operand * op = (Operand *) malloc(sizeof(Operand));
  op->kind = opVARIABLE;
  op->u.var_no = findVar(id);
  assert(op->u.var_no >= 0);
  return op;
}

char * lookup_funclist(char * funcname) {
  if ( strncmp(funcname, "read", MAXID) == 0 ) return funcname;
  if ( strncmp(funcname, "write", MAXID) == 0 ) return funcname;
  assert(findFunc(funcname) >= 0);
  return funcname;
}

InterCodes * linkcode(int num, ...) {
  assert(num > 1);
  InterCodes * arg = NULL, * p = NULL, * code0 = NULL;
  va_list argptr;
  va_start(argptr, num);
  while (num > 0) {
    arg = va_arg(argptr, InterCodes *);
    if (code0 == NULL) {
      code0 = arg;
    } else {
      if (arg != NULL) {
        p = code0;
        while (p->next != NULL) p = p->next;
        p->next = arg;
        arg->prev = p;
      }
    }
    num --;
  }
  va_end(argptr);
  return code0;
}

InterCodes * gen_assign(Operand * left, Operand * right) {
  InterCodes * codeline = (InterCodes *) malloc(sizeof(InterCodes));
  codeline->prev = NULL;
  codeline->next = NULL;
  codeline->code.kind = icASSIGN;
  codeline->code.u.assign.right = right;
  codeline->code.u.assign.left = left;
  return codeline;
}

InterCodes * gen_binop(enum Terminate e, Operand * result, Operand * op1, Operand * op2) {
  InterCodes * codeline = (InterCodes *) malloc(sizeof(InterCodes));
  codeline->prev = NULL;
  codeline->next = NULL;
  switch (e) {
    case ePLUS:
      codeline->code.kind = icADD;
      break;
    case eMINUS:
      codeline->code.kind = icSUB;
      break;
    case eSTAR:
      codeline->code.kind = icMUL;
      break;
    case eDIV:
      codeline->code.kind = icDIV;
      break;
    default :
      break;
  }
  codeline->code.u.binop.result = result;
  codeline->code.u.binop.op1 = op1;
  codeline->code.u.binop.op2 = op2;
  return codeline;
}

InterCodes * gen_label(Operand * label) {
  InterCodes * codeline = (InterCodes * ) malloc(sizeof(InterCodes));
  codeline->prev = NULL;
  codeline->next = NULL;
  codeline->code.kind = icLABEL;
  codeline->code.u.label.label = label;
  return codeline;
}

InterCodes * gen_funcdef(char * funcname) {
  InterCodes * codeline = (InterCodes * ) malloc(sizeof(InterCodes));
  codeline->prev = NULL;
  codeline->next = NULL;
  codeline->code.kind = icFUNCDEF;
  strncpy(codeline->code.u.funcdef.funcname, funcname, MAXID);
  return codeline;
}

InterCodes * gen_gotobranch(Operand * label) {
  InterCodes * codeline = (InterCodes * ) malloc(sizeof(InterCodes));
  codeline->prev = NULL;
  codeline->next = NULL;
  codeline->code.kind = icGOTOBRANCH;
  codeline->code.u.gotobranch.label = label;
  return codeline;
}

InterCodes * gen_ifbranch(Operand * op1, Operand * relop, Operand * op2, Operand * label) {
  InterCodes * codeline = (InterCodes * ) malloc(sizeof(InterCodes));
  codeline->prev = NULL;
  codeline->next = NULL;
  codeline->code.kind = icIFBRANCH;
  codeline->code.u.ifbranch.op1 = op1;
  codeline->code.u.ifbranch.relop = relop;
  codeline->code.u.ifbranch.op2 = op2;
  codeline->code.u.ifbranch.label = label;
  return codeline;
}

InterCodes * gen_funcreturn(Operand * op) {
  InterCodes * codeline = (InterCodes *) malloc(sizeof(InterCodes));
  codeline->prev = NULL;
  codeline->next = NULL;
  codeline->code.kind = icFUNCRETURN;
  codeline->code.u.funcreturn.op = op;
  return codeline;
}

InterCodes * gen_memdec(Operand * op, int size) {
  InterCodes * codeline = (InterCodes *) malloc(sizeof(InterCodes));
  codeline->prev = NULL;
  codeline->next = NULL;
  codeline->code.kind = icMEMDEC;
  codeline->code.u.memdec.op = op;
  codeline->code.u.memdec.size = size;
  return codeline;
}

InterCodes * gen_arg(Operand * arg) {
  InterCodes * codeline = (InterCodes *) malloc(sizeof(InterCodes));
  codeline->prev = NULL;
  codeline->next = NULL;
  codeline->code.kind = icARG;
  codeline->code.u.arg.arg = arg;
  return codeline;
}

InterCodes * gen_call(Operand * returnop, char *  funcname) {
  InterCodes * codeline = (InterCodes *) malloc(sizeof(InterCodes));
  codeline->prev = NULL;
  codeline->next = NULL;
  codeline->code.kind = icCALL;
  codeline->code.u.call.returnop = returnop;
  strncpy(codeline->code.u.call.funcname, funcname, MAXID);
  return codeline;
}

InterCodes * gen_param(Operand * param) {
  InterCodes * codeline = (InterCodes *) malloc(sizeof(InterCodes));
  codeline->prev = NULL;
  codeline->next = NULL;
  codeline->code.kind = icPARAM;
  codeline->code.u.param.param = param;
  return codeline;
}

InterCodes * gen_read(Operand * op) {
  InterCodes * codeline = (InterCodes *) malloc(sizeof(InterCodes));
  codeline->prev = NULL;
  codeline->next = NULL;
  codeline->code.kind = icREAD;
  codeline->code.u.read.op = op;
  return codeline;
}

InterCodes * gen_write(Operand * op) {
  InterCodes * codeline = (InterCodes *) malloc(sizeof(InterCodes));
  codeline->prev = NULL;
  codeline->next = NULL;
  codeline->code.kind = icWRITE;
  codeline->code.u.write.op = op;
  return codeline;
}

InterCodes * translate_Dec(node * dec, FieldList ** sym_table, Type * type) {
  if (dec->child->sibling == NULL) {
    // VarDec
    node * vardec = dec->child;
    Type * idtype = NULL;
    if (vardec->child->label == NODE_ID) {
      // (VarDec -> ID)
      printf("VarDec -> ID\n");
      idtype = varlist[findVar(vardec->child->nvalue.value_id)]->type;
      if (idtype->kind == basic) return NULL;
      return gen_memdec(lookup_varlist(vardec->child->nvalue.value_id), get_typesize(idtype));
    } else {
      // (VarDec -> VarDec LB INT RB)
      printf("VarDec -> VarDec LB INT RB\n");
      assert(vardec->child->child->label == NODE_ID);
      idtype = varlist[findVar(vardec->child->child->nvalue.value_id)]->type;
      return gen_memdec(lookup_varlist(vardec->child->child->nvalue.value_id), get_typesize(idtype));
    }
  }
  if (dec->child->sibling->label == NODE_TERMINATE && dec->child->sibling->ntype.type_term == eASSIGNOP) {
    // VarDec ASSIGNOP Exp
    node * vardec = dec->child;
    Operand * t1 = new_temp();
    assert(vardec->child->label == NODE_ID);
    InterCodes * code1 = translate_Exp(vardec->sibling->sibling, sym_table, t1);
    return linkcode(2, code1, gen_assign(lookup_varlist(vardec->child->nvalue.value_id), t1));
  }
  return NULL;
}

InterCodes * translate_DecList(node * declist, FieldList ** sym_table, Type * type) {
  if (declist->child->sibling == NULL) {
    // Dec
    return translate_Dec(declist->child, sym_table, type);
  }
  if (declist->child->sibling->label == NODE_TERMINATE && declist->child->sibling->ntype.type_term == eCOMMA) {
    //Dec COMMA DecList
    node * dec = declist->child;
    InterCodes * code1 = translate_Dec(dec, sym_table, type);
    InterCodes * code2 = translate_DecList(dec->sibling, sym_table, type);
    return linkcode(2, code1, code2);
  }
  return NULL;
}

InterCodes * translate_Def(node * def, FieldList ** sym_table) {
  // Specifier DecList SEMI
  return translate_DecList(def->child->sibling, sym_table, NULL);
}

InterCodes * translate_DefList(node * deflist, FieldList ** sym_table) {
  printf(" deflist ntype.type_nonterm is %s\n", stringNonTerminate[deflist->ntype.type_nonterm]);
  if (deflist->child == NULL) {
    // Empty
    return NULL;
  }
  if (deflist->child->label == NODE_NONTERMINATE && deflist->child->ntype.type_nonterm == Def) {
    // Def DefList
    InterCodes * code1 = translate_Def(deflist->child, sym_table);
    InterCodes * code2 = translate_DefList(deflist->child->sibling, sym_table);
    return linkcode(2, code1, code2);
  }
  return NULL;
}

InterCodes * translate_Args(node * args, FieldList ** sym_table, Operands ** arg_list) {
  if (args->child->sibling == NULL) {
    // Exp
    Operand * t1 = new_temp();
    InterCodes * code1 = translate_Exp(args->child, sym_table, t1);
    *arg_list = insert_arglist_head(*arg_list, t1);
    return code1;
  }
  if (args->child->sibling != NULL) {
    // Exp COMMA Args
    node * p = args->child;
    Operand * t1 = new_temp();
    InterCodes * code1 = translate_Exp(p, sym_table, t1);
    *arg_list = insert_arglist_head(*arg_list, t1);
    InterCodes * code2 = translate_Args(p->sibling->sibling, sym_table, arg_list);
    return linkcode(2, code1, code2);
  }
  return NULL;
}

InterCodes * translate_Exp(node * exp, FieldList ** sym_table, Operand * place) {
  if (exp->child->label == NODE_INT && exp->child->sibling == NULL) {
    // INT
    Operand * value = get_value(exp->child->nvalue.value_int);
    return (place != NULL) ? gen_assign(place, value) : NULL;
  }
  if (exp->child->label == NODE_ID && exp->child->sibling == NULL) {
    // ID
    Operand * variable = lookup_varlist(exp->child->nvalue.value_id);
    return (place != NULL) ? gen_assign(place, variable) : NULL;
  }
  if (exp->child->label == NODE_ID
      && exp->child->sibling->label == NODE_TERMINATE && exp->child->sibling->ntype.type_term == eLP
      && exp->child->sibling->sibling->label == NODE_TERMINATE
      && exp->child->sibling->sibling->ntype.type_term == eRP) {
    // ID LP RP (function)
    node * p = exp->child;
    char * function = lookup_funclist(p->nvalue.value_id);
    if ( strncmp(function, "read", MAXID) == 0 ) return gen_read(place);
    return gen_call(place, function);
  }
  if (exp->child->label == NODE_ID
      && exp->child->sibling->label == NODE_TERMINATE && exp->child->sibling->ntype.type_term == eLP
      && exp->child->sibling->sibling->label == NODE_NONTERMINATE && exp->child->sibling->sibling->ntype.type_nonterm == Args) {
    // ID LP Args RP (function)
    node * p = exp->child;
    char * function = lookup_funclist(p->nvalue.value_id);
    Operands * arg_list = NULL;
    InterCodes * code1 = translate_Args(p->sibling->sibling, sym_table, &arg_list);
    if ( strncmp(function, "write", MAXID) == 0 ) 
      return linkcode(2, code1, gen_write(get_arglist(arg_list, 0)));
    InterCodes * code2 = NULL;
    int i = 0;
    Operand * op;
    assert(arg_list != NULL);
    while ( (op = get_arglist(arg_list, i)) != NULL ) {
      linkcode(2, code2, gen_arg(op));
      i ++;
    }
    return linkcode(3, code1, code2, gen_call(place, function));
  }
  if (exp->child->label == NODE_TERMINATE && exp->child->ntype.type_term == eMINUS
      && exp->child->sibling->sibling == NULL) {
    // MINUS Exp
    Operand * t1 = new_temp();
    InterCodes * code1 = translate_Exp(exp->child->sibling, varlist, t1);
    InterCodes * code2 = gen_binop(eMINUS, place, get_value(0), t1);
    linkcode(2, code1, code2);
    return code1;
  }
  if (exp->child->sibling->label == NODE_TERMINATE && exp->child->sibling->ntype.type_term == eASSIGNOP) {
    // Exp ASSIGNOP Exp
    node * p = exp->child;
    Operand * t1 = new_temp();
    assert(p->child->label == NODE_ID);
    Operand * variable = lookup_varlist(p->child->nvalue.value_id);
    InterCodes * code1 = translate_Exp(p->sibling->sibling, varlist, t1);
    InterCodes * code2 =
      linkcode(2, gen_assign(variable, t1), ((place != NULL) ? gen_assign(place, variable) : NULL) );
    return linkcode(2, code1, code2);
  }
  if (exp->child->sibling->label == NODE_TERMINATE 
      && (exp->child->sibling->ntype.type_term == ePLUS ||
      exp->child->sibling->ntype.type_term == eMINUS ||
      exp->child->sibling->ntype.type_term == eSTAR ||
      exp->child->sibling->ntype.type_term == eDIV) ) {
    // Exp PLUS/MINUS/STAR/DIV Exp
    Operand * t1 = new_temp();
    Operand * t2 = new_temp();
    InterCodes * code1 = translate_Exp(exp->child, varlist, t1);
    InterCodes * code2 = translate_Exp(exp->child->sibling->sibling, varlist, t2);
    InterCodes * code3 = gen_binop(exp->child->sibling->ntype.type_term, place, t1, t2);
    return linkcode(3, code1, code2, code3);
  }
  if ( (exp->child->label == NODE_TERMINATE && exp->child->ntype.type_term == eNOT) 
      || exp->child->sibling->label == NODE_RELOP
      || (exp->child->sibling->label == NODE_TERMINATE && exp->child->sibling->ntype.type_term == eAND)
      || (exp->child->sibling->label == NODE_TERMINATE && exp->child->sibling->ntype.type_term == eOR) ) {
    // Exp RELOP/AND/OR Exp / NOT Exp
    Operand * label1 = new_label();
    Operand * label2 = new_label();
    InterCodes * code0 = (place != NULL) ? gen_assign(place, get_value(0)) : NULL;
    InterCodes * code1 = translate_Cond(exp, label1, label2, varlist);
    InterCodes * code2 =
      linkcode(2, gen_label(label1), ((place != NULL) ? gen_assign(place, get_value(0)) : NULL) );
    return linkcode(4, code0, code1, code2, gen_label(label2));
  }
  if (exp->child->label == NODE_TERMINATE && exp->child->ntype.type_term == eLP) {
    // LP Exp RP
    return translate_Exp(exp->child->sibling, sym_table, place);
  }
  if (exp->child->label == NODE_NONTERMINATE && exp->child->ntype.type_nonterm == Exp
      && exp->child->sibling->label == NODE_TERMINATE && exp->child->sibling->ntype.type_term == eDOT) {
    // Exp DOT ID (structure)
    return NULL;
  }
  if (exp->child->label == NODE_NONTERMINATE && exp->child->ntype.type_nonterm == Exp
      && exp->child->sibling->label == NODE_TERMINATE && exp->child->sibling->ntype.type_term == eLB) {
    // Exp LB Exp RB (array)
    return NULL;
  }
  return NULL;
}

InterCodes * translate_Cond(node * exp, Operand * label_true, Operand * label_false, FieldList ** sym_table) {
  if (exp->child->label == NODE_TERMINATE && exp->child->ntype.type_term == eNOT) {
    // NOT Exp
    return translate_Cond(exp->child->sibling, label_false, label_true, sym_table);
  }
  if ( exp->child->sibling->label == NODE_RELOP) {
    // Exp RELOP Exp
    node * p = exp->child;
    Operand * t1 = new_temp();
    Operand * t2 = new_temp();
    InterCodes * code1 = translate_Exp(p, sym_table, t1);
    InterCodes * code2 = translate_Exp(p->sibling->sibling, sym_table, t2);
    Operand * op = get_relop(exp->child->sibling->nvalue.value_relop);
    InterCodes * code3 = gen_ifbranch(t1, op, t2, label_true);
    return linkcode(4, code1, code2, code3, gen_gotobranch(label_false));
  }
  if (exp->child->sibling->label == NODE_TERMINATE && exp->child->sibling->ntype.type_term == eAND) {
    // Exp AND Exp
    node * p = exp->child;
    Operand * label1 = new_label();
    InterCodes * code1 = translate_Cond(p, label1, label_false, sym_table);
    InterCodes * code2 = translate_Cond(p->sibling->sibling, label_true, label_false, sym_table);
    return linkcode(3, code1, gen_gotobranch(label1), code2);
  }
  if (exp->child->sibling->label == NODE_TERMINATE && exp->child->sibling->ntype.type_term == eOR) {
    // Exp OR Exp
    node * p = exp->child;
    Operand * label1 = new_label();
    InterCodes * code1 = translate_Cond(p, label_true, label1, sym_table);
    InterCodes * code2 = translate_Cond(p->sibling->sibling, label_true, label_false, sym_table);
    return linkcode(3, code1, gen_gotobranch(label1), code2);
  }
  if (true) {
    // [other cases]
    Operand * t1 = new_temp();
    InterCodes * code1 = translate_Exp(exp, sym_table, t1);
    InterCodes * code2 = gen_ifbranch(t1, get_relop(eNE), get_value(0), label_true);
    return linkcode(3, code1, code2, gen_gotobranch(label_false));
  }
  return NULL;
}

InterCodes * translate_Stmt(node * stmt, FieldList ** sym_table) {
  if (stmt->child->label == NODE_NONTERMINATE && stmt->child->ntype.type_nonterm == CompSt) {
    // CompSt
    return translate_CompSt(stmt->child, sym_table);
  }
  if (stmt->child->label == NODE_NONTERMINATE && stmt->child->ntype.type_nonterm == Exp
      && stmt->child->sibling->label == NODE_TERMINATE && stmt->child->sibling->ntype.type_term == eSEMI) {
    // Exp SEMI
    return translate_Exp(stmt->child, sym_table, NULL);
  }
  if (stmt->child->label == NODE_TERMINATE && stmt->child->ntype.type_term == eRETURN) {
    // RETURN Exp SEMI
    node * p = stmt->child;
    Operand * t1 = new_temp();
    InterCodes * code1 = translate_Exp(p->sibling, sym_table, t1);
    InterCodes * code2 = gen_funcreturn(t1);
    linkcode(2, code1, code2);
    return code1;
  }
  if (stmt->child->label == NODE_TERMINATE && stmt->child->ntype.type_term == eWHILE) {
    // WHILE LP Exp RP Stmt
    node * p = stmt->child;
    Operand * label1 = new_label();
    Operand * label2 = new_label();
    Operand * label3 = new_label();
    InterCodes * code1 = translate_Cond(p->sibling->sibling, label2, label3, sym_table);
    InterCodes * code2 = translate_Stmt(p->sibling->sibling->sibling->sibling, sym_table);
    return linkcode(6, gen_label(label1), code1, gen_label(label2), code2, gen_gotobranch(label1), gen_label(label3));
  }
  if (stmt->child->label == NODE_TERMINATE && stmt->child->ntype.type_term == eIF
      && stmt->child->sibling->sibling->sibling->sibling->sibling == NULL) {
    // IF LP Exp RP Stmt
    node * p = stmt->child;
    Operand * label1 = new_label();
    Operand * label2 = new_label();
    InterCodes * code1 = translate_Cond(p->sibling->sibling, label1, label2, sym_table);
    InterCodes * code2 = translate_Stmt(p->sibling->sibling->sibling->sibling, sym_table);
    return linkcode(4, code1, gen_label(label1), code2, gen_label(label2));
  }
  if (stmt->child->label == NODE_TERMINATE && stmt->child->ntype.type_term == eIF
      && stmt->child->sibling->sibling->sibling->sibling->sibling->label == NODE_TERMINATE) {
    // IF LP Exp RP Stmt ELSE Stmt
    node * p = stmt->child;
    Operand * label1 = new_label();
    Operand * label2 = new_label();
    Operand * label3 = new_label();
    InterCodes * code1 = translate_Cond(p->sibling->sibling, label1, label2, sym_table);
    InterCodes * code2 = translate_Stmt(p->sibling->sibling->sibling->sibling, sym_table);
    InterCodes * code3 = translate_Stmt(p->sibling->sibling->sibling->sibling->sibling->sibling, sym_table);
    return linkcode(7, code1, gen_label(label1), code2, gen_gotobranch(label3), gen_label(label2), code3, gen_label(label3));
  }
  return NULL;
}

InterCodes * translate_StmtList(node * stmtlist, FieldList ** sym_table) {
  if (stmtlist->child == NULL) {
    // Empty
    return NULL;
  }
  if (stmtlist->child->label == NODE_NONTERMINATE && stmtlist->child->ntype.type_nonterm != Empty) {
    // Stmt StmtList
    InterCodes * code1 = translate_Stmt(stmtlist->child, sym_table);
    InterCodes * code2 = translate_StmtList(stmtlist->child->sibling, sym_table);
    return linkcode(2, code1, code2);
  }
  return NULL;
}

InterCodes * translate_ParamDec(node * paramdec, FieldList ** sym_table) {
  // Specifier VarDec
  node * vardec = paramdec->child->sibling;
  Operand * param = NULL;
  if (vardec->child->label == NODE_ID) {
    // (VarDec -> ID)
    param = lookup_varlist(vardec->child->nvalue.value_id);
  } else {
    // (VarDec -> VarDec LB INT RB)
    assert(vardec->child->child->label == NODE_ID);
    param = lookup_varlist(vardec->child->child->nvalue.value_id);
  }
  return gen_param(param);
}

InterCodes * translate_VarList(node * varlist , FieldList ** sym_table) {
  if (varlist->child->sibling == NULL) {
    // ParamDec
    return translate_ParamDec(varlist->child, sym_table);
  }
  if (varlist->child->sibling->label == NODE_TERMINATE && varlist->child->sibling->ntype.type_term == eCOMMA) {
    // ParamDec COMMA Varlist
    InterCodes * code1 = translate_ParamDec(varlist->child, sym_table);
    InterCodes * code2 = translate_VarList(varlist->child->sibling->sibling, sym_table);
    return linkcode(2, code1, code2);
  }
}

InterCodes * translate_FunDec(node * fundec, FieldList ** sym_table) {
  if (fundec->child->sibling->sibling->label == NODE_TERMINATE
      && fundec->child->sibling->sibling->ntype.type_term == eRP) {
    // ID LP RP
    return gen_funcdef(fundec->child->nvalue.value_id);
  }
  if (fundec->child->sibling->sibling->label == NODE_NONTERMINATE
      && fundec->child->sibling->sibling->ntype.type_nonterm == VarList) {
    // ID LP VarList RP
    node * p = fundec->child;
    InterCodes * code1 = gen_funcdef(p->nvalue.value_id);
    InterCodes * code2 = translate_VarList(p->sibling->sibling, sym_table);
    return linkcode(2, code1, code2);
  }
  return NULL;
}
    
InterCodes * translate_CompSt(node * compst, FieldList ** sym_table) {
  // LC DefList StmtList RC
  InterCodes * code1 = translate_DefList(compst->child->sibling, sym_table);
  InterCodes * code2 = translate_StmtList(compst->child->sibling->sibling, sym_table);
  return linkcode(2, code1, code2);
}

InterCodes * translate_ExtDef(node * extdef, FieldList ** sym_table) {
  if (extdef->child->sibling->label == NODE_TERMINATE && extdef->child->sibling->ntype.type_term == eSEMI) {
    // Specifier SEMI
    return NULL;
  }
  if (extdef->child->sibling->label == NODE_NONTERMINATE && extdef->child->sibling->ntype.type_nonterm == FunDec) {
    // Specifier FunDec CompSt (function)
    node * p = extdef->child;
    InterCodes * code1 = translate_FunDec(p->sibling, sym_table);
    InterCodes * code2 = translate_CompSt(p->sibling->sibling, sym_table);
    return linkcode(2, code1, code2);
  }
  if (extdef->child->sibling->label == NODE_NONTERMINATE && extdef->child->sibling->ntype.type_nonterm == ExtDecList) {
    // Specifier ExtDecList SEMI (global variable)
    return translate_DecList(extdef->child->sibling, sym_table, NULL);
  }
  return NULL;
}

InterCodes * translate_ExtDefList(node * extdeflist, FieldList ** sym_table) {
  if (extdeflist->child == NULL) {
    // Empty
    return NULL;
  }
  if (extdeflist->child->label == NODE_NONTERMINATE && extdeflist->child->ntype.type_nonterm == ExtDef) {
    // ExtDef ExtDefList
    node * extdef = extdeflist->child;
    InterCodes * code1 = translate_ExtDef(extdef, sym_table);
    InterCodes * code2 = translate_ExtDefList(extdef->sibling, sym_table);
    return linkcode(2, code1, code2);
  }
  return NULL;
}    

void translate(node * p) {
  if (p != NULL) {
    if (p->label == NODE_NONTERMINATE ) {
      printf("%s (%d)\n", stringNonTerminate[p->ntype.type_nonterm], p->lineno);
      switch (p->ntype.type_nonterm) {
        case ExtDefList:
          ichead = linkcode(2, ichead, translate_ExtDefList(p, varlist));
          break;
        default:
          translate(p->child);
          translate(p->sibling);
          break;
      }
    } else {
      translate(p->child);
      translate(p->sibling);
    }
  }
}

void printoperand(Operand * op) {
  if (op != NULL)
  switch (op->kind) {
    case opVARIABLE:
      printf(" var%d ", op->u.var_no );
      break;
    case opCONSTANT:
      printf(" #%d ", op->u.value );
      break;
    case opADDRESS:
      printf(" addr%d ", op->u.addr_no );
      break;
    case opTEMP:
      printf(" t%d ", op->u.temp_no );
      break;
    case opLABEL:
      printf(" label%d ", op->u.var_no );
      break;
    case opRELOP:
      printf(" %s ", stringRelopValue[op->u.relop]);
      break;
    default :
      break;
  }
}

void printcode(InterCodes * code) {
  if (code == NULL) {
    printf("no intermediate code\n");
    return ;
  }
  InterCodes * p = code;
  while (p != NULL) {
    switch (p->code.kind) {
      case icASSIGN:
        printoperand(p->code.u.assign.left);
        printf(" := ");
        printoperand(p->code.u.assign.right);
        break;
      case icADD:
        printoperand(p->code.u.binop.result);
        printf(" := ");
        printoperand(p->code.u.binop.op1);
        printf(" + ");
        printoperand(p->code.u.binop.op2);
        break;
      case icSUB:
        printoperand(p->code.u.binop.result);
        printf(" := ");
        printoperand(p->code.u.binop.op1);
        printf(" - ");
        printoperand(p->code.u.binop.op2);
        break;
      case icMUL:
        printoperand(p->code.u.binop.result);
        printf(" := ");
        printoperand(p->code.u.binop.op1);
        printf(" * ");
        printoperand(p->code.u.binop.op2);
        break;
      case icDIV:
        printoperand(p->code.u.binop.result);
        printf(" := ");
        printoperand(p->code.u.binop.op1);
        printf(" / ");
        printoperand(p->code.u.binop.op2);
        break;
      case icLABEL:
        printf(" LABEL ");
        printoperand(p->code.u.label.label);
        printf(" : ");
        break;
      case icFUNCDEF:
        printf(" FUNCTION %s :", p->code.u.funcdef.funcname);
        break;
      case icGETADDR:
        break;
      case icMEMREAD:
        break;
      case icMEMWRITE:
        break;
      case icGOTOBRANCH:
        printf(" GOTO ");
        printoperand(p->code.u.gotobranch.label);
        break;
      case icIFBRANCH:
        printf(" IF ");
        printoperand(p->code.u.ifbranch.op1);
        printoperand(p->code.u.ifbranch.relop);
        printoperand(p->code.u.ifbranch.op2);
        printf(" GOTO ");
        printoperand(p->code.u.ifbranch.label);
        break;
      case icFUNCRETURN:
        printf(" RETURN ");
        printoperand(p->code.u.funcreturn.op);
        break;
      case icMEMDEC:
        printf(" DEC ");
        printoperand(p->code.u.memdec.op);
        printf(" %d ", p->code.u.memdec.size);
        break;
      case icARG:
        printf(" ARG ");
        printoperand(p->code.u.arg.arg);
        break;
      case icCALL:
        printoperand(p->code.u.call.returnop);
        printf(" := CALL %s ", p->code.u.call.funcname);
        break;
      case icPARAM:
        printf(" PARAM ");
        printoperand(p->code.u.param.param);
        break;
      case icREAD:
        printf(" READ ");
        printoperand(p->code.u.read.op);
        break;
      case icWRITE:
        printf(" WRITE ");
        printoperand(p->code.u.write.op);
        break;
      default :
        break;
    }
    printf("\n");
    p = p->next;
  }
  return ;
}

void fprintoperand(FILE * file, Operand * op) {
  if (op != NULL)
  switch (op->kind) {
    case opVARIABLE:
      fprintf(file, " var%d ", op->u.var_no );
      break;
    case opCONSTANT:
      fprintf(file, " #%d ", op->u.value );
      break;
    case opADDRESS:
      fprintf(file, " addr%d ", op->u.addr_no );
      break;
    case opTEMP:
      fprintf(file, " t%d ", op->u.temp_no );
      break;
    case opLABEL:
      fprintf(file, " label%d ", op->u.var_no );
      break;
    case opRELOP:
      fprintf(file, " %s ", stringRelopValue[op->u.relop]);
      break;
    default :
      break;
  }
}

void fprintcode(FILE * file, InterCodes * code) {
  if (code == NULL) {
    fprintf(file, "no intermediate code\n");
    return ;
  }
  InterCodes * p = code;
  while (p != NULL) {
    switch (p->code.kind) {
      case icASSIGN:
        fprintoperand(file, p->code.u.assign.left);
        fprintf(file, " := ");
        fprintoperand(file, p->code.u.assign.right);
        break;
      case icADD:
        fprintoperand(file, p->code.u.binop.result);
        fprintf(file, " := ");
        fprintoperand(file, p->code.u.binop.op1);
        fprintf(file, " + ");
        fprintoperand(file, p->code.u.binop.op2);
        break;
      case icSUB:
        fprintoperand(file, p->code.u.binop.result);
        fprintf(file, " := ");
        fprintoperand(file, p->code.u.binop.op1);
        fprintf(file, " - ");
        fprintoperand(file, p->code.u.binop.op2);
        break;
      case icMUL:
        fprintoperand(file, p->code.u.binop.result);
        fprintf(file, " := ");
        fprintoperand(file, p->code.u.binop.op1);
        fprintf(file, " * ");
        fprintoperand(file, p->code.u.binop.op2);
        break;
      case icDIV:
        fprintoperand(file, p->code.u.binop.result);
        fprintf(file, " := ");
        fprintoperand(file, p->code.u.binop.op1);
        fprintf(file, " / ");
        fprintoperand(file, p->code.u.binop.op2);
        break;
      case icLABEL:
        fprintf(file, " LABEL ");
        fprintoperand(file, p->code.u.label.label);
        fprintf(file, " : ");
        break;
      case icFUNCDEF:
        fprintf(file, " FUNCTION %s :", p->code.u.funcdef.funcname);
        break;
      case icGETADDR:
        break;
      case icMEMREAD:
        break;
      case icMEMWRITE:
        break;
      case icGOTOBRANCH:
        fprintf(file, " GOTO ");
        fprintoperand(file, p->code.u.gotobranch.label);
        break;
      case icIFBRANCH:
        fprintf(file, " IF ");
        fprintoperand(file, p->code.u.ifbranch.op1);
        fprintoperand(file, p->code.u.ifbranch.relop);
        fprintoperand(file, p->code.u.ifbranch.op2);
        fprintf(file, " GOTO ");
        fprintoperand(file, p->code.u.ifbranch.label);
        break;
      case icFUNCRETURN:
        fprintf(file, " RETURN ");
        fprintoperand(file, p->code.u.funcreturn.op);
        break;
      case icMEMDEC:
        fprintf(file, " DEC ");
        fprintoperand(file, p->code.u.memdec.op);
        fprintf(file, " %d ", p->code.u.memdec.size);
        break;
      case icARG:
        fprintf(file, " ARG ");
        fprintoperand(file, p->code.u.arg.arg);
        break;
      case icCALL:
        fprintoperand(file, p->code.u.call.returnop);
        fprintf(file, " := CALL %s ", p->code.u.call.funcname);
        break;
      case icPARAM:
        fprintf(file, " PARAM ");
        fprintoperand(file, p->code.u.param.param);
        break;
      case icREAD:
        fprintf(file, " READ ");
        fprintoperand(file, p->code.u.read.op);
        break;
      case icWRITE:
        fprintf(file, " WRITE ");
        fprintoperand(file, p->code.u.write.op);
        break;
      default :
        break;
    }
    fprintf(file, "\n");
    p = p->next;
  }
  return ;
}

void writecode() {
  FILE * file = fopen("test01.ir", "w");
  if (file == NULL) {
    perror("Open file failed\n");
    exit(0);
  }
  fprintcode(file, ichead);
  fclose(file);
}
