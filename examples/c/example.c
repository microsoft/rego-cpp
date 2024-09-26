#include <rego/rego_c.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
  regoEnum err;
  int rc = EXIT_SUCCESS;
  regoOutput* output = NULL;
  regoInterpreter* rego = regoNew();

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

  printf("%s\n", regoOutputString(output));
  goto exit;

error:
  printf("%s\n", regoGetError(rego));
  rc = EXIT_FAILURE;

exit:
  if (output != NULL)
  {
    regoFreeOutput(output);
  }

  if (rego != NULL)
  {
    regoFree(rego);
  }

  return rc;
}
