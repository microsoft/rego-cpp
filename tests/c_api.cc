#include <cstdio>
#include <cstdlib>
#include <rego/rego_c.h>

const char* OBJECTS = R"(package objects

rect := {`width`: 2, "height": 4}
cube := {"width": 3, `height`: 4, "depth": 5}
a := 42
b := false
c := null
d := {"a": a, "x": [b, c]}
index := 1
shapes := [rect, cube]
names := ["prod", `smoke1`, "dev"]
sites := [{"name": "prod"}, {"name": names[index]}, {"name": "dev"}]
e := {
    a: "foo",
    "three": c,
    names[2]: b,
    "four": d,
}
f := e["dev"]
)";

const char* DATA0 = R"({
    "one": {
        "bar": "Foo",
        "baz": 5,
        "be": true,
        "bop": 23.4
    },
    "two": {
        "bar": "Bar",
        "baz": 12.3,
        "be": false,
        "bop": 42
    }
}
)";

const char* DATA1 = R"({
    "three": {
        "bar": "Baz",
        "baz": 15,
        "be": true,
        "bop": 4.23
    }
}
)";

const char* INPUT0 = R"({
    "a": 10,
    "b": "20",
    "c": 30.0,
    "d": true
}
)";

int main()
{
  regoEnum err;
  int rc = REGO_OK;
  regoOutput* output = NULL;
  regoNode* node = NULL;
  regoInterpreter* rego = regoNew();
  regoSize size = 0;
  char* buf = NULL;

  err = regoAddModule(rego, "objects.rego", OBJECTS);
  if (err != REGO_OK)
  {
    goto error;
  }

  err = regoAddDataJSON(rego, DATA0);
  if (err != REGO_OK)
  {
    goto error;
  }

  err = regoAddDataJSON(rego, DATA1);
  if (err != REGO_OK)
  {
    goto error;
  }

  err = regoSetInputTerm(rego, INPUT0);
  if (err != REGO_OK)
  {
    goto error;
  }

  output = regoQuery(rego, "[data.one, input.b, data.objects.sites[1]] = x");
  if (output == NULL)
  {
    goto error;
  }

  printf("%s\n", regoOutputString(output));

  node = regoOutputBinding(output, "x");
  if (node == NULL)
  {
    goto error;
  }

  size = regoNodeJSONSize(node);
  if (size == 0)
  {
    goto error;
  }

  buf = (char*)malloc(size);
  err = regoNodeJSON(node, buf, size);
  if (err != REGO_OK)
  {
    goto error;
  }

  printf("x = %s\n", buf);
  free(buf);
  goto exit;

error:
  printf("%s\n", regoGetError(rego));
  rc = REGO_ERROR;

exit:
  if (output != nullptr)
  {
    regoFreeOutput(output);
  }

  if (rego != nullptr)
  {
    regoFree(rego);
  }

  return rc;
}