#include "context.h"
#include "conversions.h"
#include "error.h"
#include "extensions.h"
#include "idhash.h"

static JSBool define_property(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *retval);

static JSBool global_enumerate(JSContext *js_context, JSObject *obj)
{
  return JS_EnumerateStandardClasses(js_context, obj);
}

static JSBool global_resolve(
  JSContext *js_context, JSObject *obj, jsval id, uintN flags, JSObject **objp)
{
  if ((flags & JSRESOLVE_ASSIGNING) == 0) {
    JSBool resolved_p;

    if (!JS_ResolveStandardClass(js_context, obj, id, &resolved_p))
      return JS_FALSE;
    if (resolved_p) *objp = obj;
  }

  return JS_TRUE;
}

static JSClass OurGlobalClass = {
  "global", JSCLASS_NEW_RESOLVE | JSCLASS_GLOBAL_FLAGS,
  JS_PropertyStub, // addProperty
  JS_PropertyStub, // delProperty
  JS_PropertyStub, // getProperty
  JS_PropertyStub, // setProperty
  global_enumerate,
  (JSResolveOp) global_resolve,
  JS_ConvertStub,
  JS_FinalizeStub,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

static VALUE global(VALUE self)
{
  OurContext* context;
  Data_Get_Struct(self, OurContext, context);
  return convert_to_ruby(context, OBJECT_TO_JSVAL(context->global));
}

static VALUE evaluate(int argc, VALUE* argv, VALUE self)
{
  VALUE script, filename, linenum;

  OurContext* context;
  Data_Get_Struct(self, OurContext, context);

  rb_scan_args( argc, argv, "12", &script, &filename, &linenum );

  // clean things up first
  context->ex = 0;
  memset(context->msg, 0, MAX_EXCEPTION_MESSAGE_SIZE);

  char* filenamez = RTEST(filename) ? StringValueCStr(filename) : NULL;
  int linenumi = RTEST(linenum) ? NUM2INT(linenum) : 1;

  jsval js;

  // FIXME: should be able to pass in the 'file' name
  JSBool ok = JS_EvaluateScript(context->js, context->global,
    StringValuePtr(script), StringValueLen(script), filenamez, linenumi, &js);

  if (!ok)
  {
    if (JS_IsExceptionPending(context->js))
    {
      // If there's an exception pending here, it's a syntax error.
      JS_GetPendingException(context->js, &context->ex);
      JS_ClearPendingException(context->js);
    }

    char* msg = context->msg;

    // toString() whatever the exception object is (if we have one)
    if (context->ex) msg = JS_GetStringBytes(JS_ValueToString(context->js, context->ex));

    return Johnson_Error_raise(msg);
  }

  return convert_to_ruby(context, js);
}

static void error(JSContext* js, const char* message, JSErrorReport* report)
{
  // first we find ourselves
  VALUE self = (VALUE)JS_GetContextPrivate(js);

  // then we find our bridge
  OurContext* context;
  Data_Get_Struct(self, OurContext, context);

  // NOTE: SpiderMonkey REALLY doesn't like being interrupted. If we
  // jump over to Ruby and raise here, segfaults and such ensue.
  // Instead, we store the exception (if any) and the error message
  // on the context. They're dealt with in the if (!ok) block of evaluate().

  strncpy(context->msg, message, MAX_EXCEPTION_MESSAGE_SIZE);
  JS_GetPendingException(context->js, &context->ex);
}

static void deallocate(OurContext* context)
{
  JS_SetContextPrivate(context->js, 0);

  JS_RemoveRoot(context->js, &(context->global));
  JS_RemoveRoot(context->js, &(context->gcthings));
  JS_HashTableDestroy(context->rbids);
  JS_HashTableDestroy(context->jsids);

  context->jsids = 0;
  context->rbids = 0;

  JS_DestroyContext(context->js);
  JS_DestroyRuntime(context->runtime);

  free(context);
}

static VALUE allocate(VALUE klass)
{
  OurContext* context = calloc(1, sizeof(OurContext));
  return Data_Wrap_Struct(klass, 0, deallocate, context);
}

static VALUE initialize_native(VALUE self, VALUE options) {
  OurContext* context;
  bool gthings_rooted_p = false;

  Data_Get_Struct(self, OurContext, context);

  if ((context->runtime = JS_NewRuntime(0x100000))
    && (context->js = JS_NewContext(context->runtime, 8192))
    && (context->jsids = create_id_hash())
    && (context->rbids = create_id_hash())
    && (context->gcthings = JS_NewObject(context->js, NULL, 0, 0))
    && (gthings_rooted_p = JS_AddNamedRoot(context->js, &(context->gcthings), "context->gcthings"))
    && (context->global = JS_NewObject(context->js, &OurGlobalClass, NULL, NULL))
    && (JS_AddNamedRoot(context->js, &(context->global), "context->global")))
  {
    JS_SetErrorReporter(context->js, error);
    JS_SetContextPrivate(context->js, (void *)self);

    if (init_spidermonkey_extensions(context))
      return self;

    JS_RemoveRoot(context->js, &(context->global));
  }

  if (gthings_rooted_p)
    JS_RemoveRoot(context->js, &(context->gcthings));

  if (context->rbids)
    JS_HashTableDestroy(context->rbids);

  if (context->jsids)
    JS_HashTableDestroy(context->jsids);

  if (context->js)
    JS_DestroyContext(context->js);

  if (context->runtime)
    JS_DestroyRuntime(context->runtime);

  rb_raise(rb_eRuntimeError, "Failed to initialize SpiderMonkey context");
}

void init_Johnson_SpiderMonkey_Context(VALUE spidermonkey)
{
  VALUE context = rb_define_class_under(spidermonkey, "Context", rb_cObject);

  rb_define_alloc_func(context, allocate);
  rb_define_private_method(context, "initialize_native", initialize_native, 1);

  rb_define_method(context, "global", global, 0);
  rb_define_method(context, "evaluate", evaluate, -1);
}

VALUE Johnson_SpiderMonkey_JSLandProxy()
{
  return rb_eval_string("Johnson::SpiderMonkey::JSLandProxy");
}
