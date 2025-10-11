#include <rego/rego_c.h>
#include <stdio.h>
#include <stdlib.h>

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

int print_output(const char* label, regoOutput* output)
{
  regoSize size = 0;
  char* buf = NULL;

  size = regoOutputJSONSize(output);
  if (size == 0)
  {
    return REGO_ERROR;
  }

  buf = (char*)malloc(size);
  regoEnum err = regoOutputJSON(output, buf, size);
  if (err != REGO_OK)
  {
    free(buf);
    return err;
  }

  printf("%s: %s\n", label, buf);
  free(buf);
  return REGO_OK;
}

int main()
{
  regoEnum err;
  int rc = REGO_OK;
  regoOutput* output = NULL;
  regoNode* node = NULL;
  regoBundle* bundle = NULL;
  regoInput* input = NULL;
  regoInterpreter* rego = regoNew();
  regoSize size = 0;
  char* buf = NULL;
  const char* bundle_dir = "c_api_bundle";
  const char* bundle_path = "c_api.rbb";

  regoSetDebugEnabled(rego, true);
  regoSetDebugPath(rego, "test");
  regoSetLogLevel(rego, regoLogLevelFromString("Debug"));

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

  err = print_output("Query", output);
  if (err != REGO_OK)
  {
    goto error;
  }

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
  buf = NULL;

  node = regoNodeGet(node, 1); // index
  if (node == NULL)
  {
    goto error;
  }

  size = regoNodeValueSize(node);
  if (size == 0)
  {
    goto error;
  }

  buf = (char*)malloc(size);
  err = regoNodeValue(node, buf, size);
  if (err != REGO_OK)
  {
    goto error;
  }

  printf("x[1] = `%s`\n", buf);
  free(buf);
  buf = NULL;

  regoFreeOutput(output);
  output = NULL;

  goto exit;

  err = regoSetQuery(rego, "[data.one, input.b, data.objects.sites[1]] = x");

  if (err != REGO_OK)
  {
    goto error;
  }

  err = regoAddEntrypoint(rego, "objects/sites");

  if (err != REGO_OK)
  {
    goto error;
  }

  bundle = regoBuild(rego);

  if (!regoBundleOk(bundle))
  {
    goto error;
  }

  err = regoBundleSave(rego, bundle_dir, bundle);

  if (err != REGO_OK)
  {
    goto error;
  }

  bundle = regoBundleLoad(rego, bundle_dir);

  if (bundle == NULL || !regoBundleOk(bundle))
  {
    goto error;
  }

  err = regoBundleSaveBinary(rego, bundle_path, bundle);

  if (err != REGO_OK)
  {
    goto error;
  }

  bundle = regoBundleLoadBinary(rego, bundle_path);

  if (bundle == NULL || !regoBundleOk(bundle))
  {
    goto error;
  }

  input = regoNewInput();
  regoInputString(input, "a");
  regoInputInt(input, 10);
  regoInputObjectItem(input);
  regoInputString(input, "b");
  regoInputString(input, "20");
  regoInputObjectItem(input);
  regoInputString(input, "c");
  regoInputFloat(input, 30.0);
  regoInputObjectItem(input);
  regoInputString(input, "d");
  regoInputBoolean(input, 1);
  regoInputObjectItem(input);
  regoInputObject(input, 4);

  err = regoSetInput(rego, input);

  if (err != REGO_OK)
  {
    goto error;
  }

  output = regoBundleQuery(rego, bundle);
  if (output == NULL)
  {
    goto error;
  }

  err = print_output("Bundle Query", output);
  if (err != REGO_OK)
  {
    goto error;
  }

  regoFreeOutput(output);

  output = regoBundleQueryEntrypoint(rego, bundle, "objects/sites");
  if (output == NULL)
  {
    goto error;
  }

  err = print_output("Bundle Query Endpoint", output);
  if (err != REGO_OK)
  {
    goto error;
  }

  goto exit;

error:
  if (buf != NULL)
  {
    free(buf);
    buf = NULL;
  }

  size = regoErrorSize(rego);
  if (size != 0)
  {
    buf = (char*)malloc(size);
    err = regoError(rego, buf, size);
    if (err != REGO_OK)
    {
      printf("UNKNOWN ERROR");
    }
    else
    {
      printf("ERROR: %s\n", buf);
    }
  }
  else
  {
    printf("UNKNOWN ERROR");
  }

  rc = REGO_ERROR;

exit:
  if (buf != NULL)
  {
    free(buf);
  }

  if (output != NULL)
  {
    regoFreeOutput(output);
  }

  if (bundle != NULL)
  {
    regoFreeBundle(bundle);
  }

  if (rego != NULL)
  {
    regoFree(rego);
  }

  return rc;
}