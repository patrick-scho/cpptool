#include "../qjs_utility.h"

class Test {
public:
  Test() {}
  void test1() {}
  void test2() {}
  int i, j;
};

int main(int argc, char **argv) {
  JSRuntime *rt = JS_NewRuntime();
  JSContext *ctx = JS_NewContext(rt);

  int i = 0;
  auto l = [&]() { printf("Hallo, %d\n", i++); };
  qjs::Function(ctx, "hallo", l);

  qjs::Run(ctx, "hallo()");
  qjs::Run(ctx, "hallo()");
  qjs::Run(ctx, "hallo()");
  
  qjs::Class<Test> c(ctx, "Test");
  c
    .Method("test1", &Test::test1)
    .Method("test2", &Test::test2)
    .Variable("i", &Test::i)
    .Variable("j", &Test::j);

  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return 0;
}