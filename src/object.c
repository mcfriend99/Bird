#include <stdio.h>
#include <string.h>

#include "config.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, obj_type)                                           \
  (type *)allocate_object(vm, sizeof(type), obj_type)

static b_obj *allocate_object(b_vm *vm, size_t size, b_obj_type type) {
  b_obj *object = (b_obj *)reallocate(vm, NULL, 0, size);

  object->type = type;
  object->is_marked = false;

  object->next = vm->objects;
  vm->objects = object;

#if DEBUG_MODE == 1
#if DEBUG_LOG_GC == 1
  printf("%p allocate %ld for %d\n", (void *)object, size, type);
#endif
#endif

  return object;
}

b_obj_list *new_list(b_vm *vm) {
  b_obj_list *list = ALLOCATE_OBJ(b_obj_list, OBJ_LIST);
  init_value_arr(&list->items);
  return list;
}

b_obj_bound *new_bound_method(b_vm *vm, b_value receiver, b_obj *method) {
  b_obj_bound *bound = ALLOCATE_OBJ(b_obj_bound, OBJ_BOUND_METHOD);
  bound->receiver = receiver;
  bound->method = method;
  return bound;
}

b_obj_class *new_class(b_vm *vm, b_obj_string *name) {
  b_obj_class *klass = ALLOCATE_OBJ(b_obj_class, OBJ_CLASS);
  klass->name = name;
  init_table(&klass->fields);
  init_table(&klass->methods);
  klass->initializer = EMPTY_VAL;
  return klass;
}

b_obj_func *new_function(b_vm *vm) {
  b_obj_func *function = ALLOCATE_OBJ(b_obj_func, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalue_count = 0;
  function->is_variadic = false;
  function->name = NULL;
  init_blob(&function->blob);
  return function;
}

b_obj_instance *new_instance(b_vm *vm, b_obj_class *klass) {
  b_obj_instance *instance = ALLOCATE_OBJ(b_obj_instance, OBJ_INSTANCE);
  instance->klass = klass;
  init_table(&instance->fields);
  table_add_all(vm, &klass->fields, &instance->fields);
  return instance;
}

b_obj_native *new_native(b_vm *vm, b_native_fn function, const char *name) {
  b_obj_native *native = ALLOCATE_OBJ(b_obj_native, OBJ_NATIVE);
  native->function = function;
  native->name = name;
  return native;
}

b_obj_closure *new_closure(b_vm *vm, b_obj_func *function) {
  b_obj_upvalue **upvalues = ALLOCATE(b_obj_upvalue *, function->upvalue_count);
  for (int i = 0; i < function->upvalue_count; i++) {
    upvalues[i] = NULL;
  }

  b_obj_closure *closure = ALLOCATE_OBJ(b_obj_closure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalue_count = function->upvalue_count;
  return closure;
}

b_obj_string *allocate_string(b_vm *vm, char *chars, int length,
                              uint32_t hash) {
  b_obj_string *string = ALLOCATE_OBJ(b_obj_string, OBJ_STRING);
  string->chars = chars;
  string->length = length;
  string->hash = hash;

  push(vm, OBJ_VAL(string)); // fixing gc corruption
  table_set(vm, &vm->strings, OBJ_VAL(string), NIL_VAL);
  pop(vm); // fixing gc corruption

  return string;
}

b_obj_string *take_string(b_vm *vm, char *chars, int length) {
  uint32_t hash = hash_string(chars, length);

  b_obj_string *interned = table_find_string(&vm->strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }

  return allocate_string(vm, chars, length, hash);
}

b_obj_string *copy_string(b_vm *vm, const char *chars, int length) {
  uint32_t hash = hash_string(chars, length);

  b_obj_string *interned = table_find_string(&vm->strings, chars, length, hash);
  if (interned != NULL)
    return interned;

  char *heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';

  return allocate_string(vm, heap_chars, length, hash);
}

b_obj_upvalue *new_upvalue(b_vm *vm, b_value *slot) {
  b_obj_upvalue *upvalue = ALLOCATE_OBJ(b_obj_upvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

static void print_function(b_obj_func *function) {
  if (function->name == NULL) {
    printf("<script at %p>", (void *)function);
  } else {
    printf("<function %s at %p>", function->name->chars, (void *)function);
  }
}

void print_object(b_value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_LIST: {
    printf("[");
    b_obj_list *list = AS_LIST(value);
    for (int i = 0; i < list->items.count; i++) {
      print_value(list->items.values[i]);
      if (i != list->items.count - 1) {
        printf(", ");
      }
    }
    printf("]");
    break;
  }

  case OBJ_BOUND_METHOD: {
    b_obj *method = AS_BOUND(value)->method;
    if (method->type == OBJ_CLOSURE) {
      print_function(((b_obj_closure *)method)->function);
    } else {
      print_function((b_obj_func *)method);
    }
    break;
  }
  case OBJ_CLASS: {
    printf("<class %s at %p>", AS_CLASS(value)->name->chars,
           (void *)AS_CLASS(value));
    break;
  }
  case OBJ_CLOSURE: {
    print_function(AS_CLOSURE(value)->function);
    break;
  }
  case OBJ_FUNCTION: {
    print_function(AS_FUNCTION(value));
    break;
  }
  case OBJ_INSTANCE: {
    // @TODO: support the to_string() override
    b_obj_instance *instance = AS_INSTANCE(value);
    printf("<class %s instance at %p>", instance->klass->name->chars,
           (void *)instance);
    break;
  }
  case OBJ_NATIVE: {
    b_obj_native *native = AS_NATIVE(value);
    printf("<function(native) %s at %p>", native->name, (void *)native);
    break;
  }
  case OBJ_UPVALUE: {
    printf("upvalue");
    break;
  }
  case OBJ_STRING: {
    printf("%s", AS_CSTRING(value));
    break;
  }
  }
}

const char *object_type(b_obj *object) {
  switch (object->type) {
  case OBJ_LIST:
    return "list";

  case OBJ_CLASS:
    return "class";

  case OBJ_FUNCTION:
  case OBJ_NATIVE:
  case OBJ_CLOSURE:
  case OBJ_BOUND_METHOD:
    return "function";

  case OBJ_INSTANCE:
    return ((b_obj_instance *)object)->klass->name->chars;

  case OBJ_STRING:
    return "string";

  default:
    return "unknown";
  }
}