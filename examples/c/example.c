#include <rego/rego_c.h>
#include <stdio.h>
#include <stdlib.h>

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

int print_error(regoInterpreter* rego)
{
  regoEnum err = REGO_OK;
  regoSize size = 0;
  char* buf = NULL;

  size = regoErrorSize(rego);
  if (size == 0)
  {
    return REGO_ERROR;
  }

  buf = (char*)malloc(size);
  err = regoError(rego, buf, size);
  if (err == REGO_OK)
  {
    printf("%s\n", buf);
    free(buf);
    return REGO_OK;
  }

  free(buf);
  printf("Unknown error: Unable to obtain error message\n");
  return err;
}

int main(void)
{
  regoEnum err;
  int rc = EXIT_SUCCESS;
  regoOutput* output = NULL;
  regoNode* node = NULL;
  regoBundle* bundle = NULL;
  regoInput* input = NULL;
  regoInterpreter* rego = regoNew();
  regoSize size = 0;
  char* buf = NULL;
  const char* bundle_dir = "example_bundle";
  const char* bundle_path = "example.rbb";

  err = regoAddModuleFile(rego, "examples/objects.rego");
  if (err != REGO_OK)
  {
    goto error;
  }

  err = regoAddDataJSONFile(rego, "examples/data0.json");
  if (err != REGO_OK)
  {
    goto error;
  }

  err = regoAddDataJSONFile(rego, "examples/data1.json");
  if (err != REGO_OK)
  {
    goto error;
  }

  err = regoSetInputJSONFile(rego, "examples/input0.json");
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

  regoFreeBundle(bundle);
  bundle = NULL;

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

  regoFreeBundle(bundle);
  bundle = NULL;

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
  err = regoInputValidate(input);

  if (err != REGO_OK)
  {
    goto error;
  }

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
  print_error(rego);
  rc = EXIT_FAILURE;

exit:
  if (buf != NULL)
  {
    free(buf);
  }

  if (output != NULL)
  {
    regoFreeOutput(output);
  }

  if (input != NULL)
  {
    regoFreeInput(input);
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
