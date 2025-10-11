#include <cargs.h>
#include <rego/rego_c.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DATA_FILES 128
#define MAX_VALUE_LENGTH 256

#define MODE_EVAL 0
#define MODE_BUILD 1
#define MODE_RUN 2

/**
 * This is the main configuration of all options available.
 */
static struct cag_option options[] = {
  {.identifier = 'm',
   .access_letters = "m",
   .access_name = "mode",
   .value_name = "VALUE",
   .description = "Mode (eval (default), build, run)"},

  {.identifier = 'l',
   .access_letters = "l",
   .access_name = "log_level",
   .value_name = "VALUE",
   .description = "Log level (one of 'n', 'e', 'w', 'i', 'd', 't')"},

  {.identifier = 'r',
   .access_letters = "r",
   .access_name = "raw",
   .value_name = NULL,
   .description = "Use the raw query interface (print node objects)"},

  {.identifier = 'q',
   .access_letters = "q",
   .access_name = "query",
   .value_name = "VALUE",
   .description = "Query"},

  {.identifier = 'i',
   .access_letters = "i",
   .access_name = "input",
   .value_name = "VALUE",
   .description = "Input JSON file"},

  {.identifier = 'd',
   .access_letters = "d",
   .access_name = "data",
   .value_name = "VALUE",
   .description = "Data/Policy Files"},

  {.identifier = 'b',
   .access_letters = "b",
   .access_name = "bundle",
   .value_name = "VALUE",
   .description = "Path to bundle file"},

  {.identifier = 'f',
   .access_letters = "f",
   .access_name = "format",
   .value_name = "VALUE",
   .description = "Format to use for bundle file (one of 'json', 'binary')"},

  {.identifier = 'e',
   .access_letters = "e",
   .access_name = "entrypoint",
   .value_name = "VALUE",
   .description = "Query entrypoint"},

  {.identifier = 'h',
   .access_letters = "h",
   .access_name = "help",
   .value_name = NULL,
   .description = "Shows the command help"}};

/**
 * This is a custom project configuration structure where you can store the
 * parsed information.
 */
struct regoc_configuration
{
  int mode;
  char log_level;
  bool use_raw_query;
  const char* data_files[MAX_DATA_FILES];
  unsigned int data_files_count;
  const char* input_file;
  const char* query;
  const char* bundle;
  bool binary_format;
  const char* entrypoint;
};

bool is_json(const char* file)
{
  size_t len;

  len = strlen(file);
  if (strcmp(file + len - 5, ".json") != 0)
  {
    return false;
  }

  return true;
}

void print_value(regoNode* node, const char* name)
{
  char value[MAX_VALUE_LENGTH];
  regoSize value_size;
  char* big_value;
  regoEnum err;

  value_size = regoNodeValueSize(node);
  if (value_size > MAX_VALUE_LENGTH)
  {
    big_value = malloc(value_size);
    if (big_value == NULL)
    {
      printf("Out of memory\n");
      exit(1);
    }
    err = regoNodeValue(node, big_value, value_size);
    if (err != REGO_OK)
    {
      printf("Unable to get node value\n");
      exit(1);
    }
    printf("(var %s)\n", big_value);
    free(big_value);
  }
  else
  {
    err = regoNodeValue(node, value, MAX_VALUE_LENGTH);
    if (err != REGO_OK)
    {
      printf("Unable to get node value\n");
      exit(1);
    }
    printf("(%s %s)\n", name, value);
  }
}

void print_node(regoNode* node, unsigned int indent)
{
  regoSize i;
  regoSize size;
  regoEnum type;
  regoEnum err;
  char* buf;

  for (i = 0; i < indent; ++i)
  {
    printf(" ");
  }

  if (node == NULL)
  {
    printf("<null>\n");
    return;
  }

  type = regoNodeType(node);
  switch (type)
  {
    case REGO_NODE_VAR:
      print_value(node, "var");
      return;

    case REGO_NODE_INT:
    case REGO_NODE_FLOAT:
      print_value(node, "number");
      return;

    case REGO_NODE_STRING:
      print_value(node, "string");
      return;

    case REGO_NODE_TRUE:
      printf("(boolean true)\n");
      return;

    case REGO_NODE_FALSE:
      printf("(boolean false)\n");
      return;

    case REGO_NODE_NULL:
      printf("(null)\n");
      return;

    default:
      size = regoNodeTypeNameSize(node);
      buf = malloc(size);
      err = regoNodeTypeName(node, buf, size);
      if (err == REGO_OK)
      {
        printf("(%s", buf);
      }
      else
      {
        printf("(<error>");
      }
      free(buf);
      break;
  }

  size = regoNodeSize(node);

  printf("\n");
  for (i = 0; i < size; ++i)
  {
    print_node(regoNodeGet(node, i), indent + 2);
  }

  if (size == 0)
  {
    printf(")\n");
    return;
  }
}

int print_output(regoOutput* output)
{
  regoEnum err = REGO_OK;
  regoSize size = 0;
  char* buf = NULL;

  size = regoOutputJSONSize(output);
  if (size == 0)
  {
    return REGO_ERROR;
  }

  buf = (char*)malloc(size);
  err = regoOutputJSON(output, buf, size);
  if (err != REGO_OK)
  {
    free(buf);
    return err;
  }

  printf("%s\n", buf);
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

  buf = malloc(size);
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

int main(int argc, char** argv)
{
  printf(
    "regoc %s (%s, %s) [%s] on %s\n",
    REGOCPP_VERSION,
    REGOCPP_BUILD_NAME,
    REGOCPP_BUILD_DATE,
    REGOCPP_BUILD_TOOLCHAIN,
    REGOCPP_PLATFORM);
  char identifier;
  cag_option_context context;
  struct regoc_configuration config = {
    .mode = MODE_EVAL,
    .log_level = 'n',
    .use_raw_query = false,
    .data_files_count = 0,
    .input_file = NULL,
    .query = NULL,
    .bundle = "bundle",
    .binary_format = false,
    .entrypoint = NULL};
  unsigned int data_index;
  regoInterpreter* rego = NULL;
  regoOutput* output = NULL;
  regoBundle* bundle = NULL;
  int rc = EXIT_SUCCESS;
  regoEnum err;

  cag_option_prepare(&context, options, CAG_ARRAY_SIZE(options), argc, argv);
  while (cag_option_fetch(&context))
  {
    identifier = cag_option_get(&context);
    switch (identifier)
    {
      case 'm': {
        const char* mode = cag_option_get_value(&context);
        if (strcmp(mode, "build") == 0)
        {
          config.mode = MODE_BUILD;
        }
        else if (strcmp(mode, "run") == 0)
        {
          config.mode = MODE_RUN;
        }
        break;
      }

      case 'l':
        config.log_level = cag_option_get_value(&context)[0];
        break;

      case 'r':
        config.use_raw_query = true;
        break;

      case 'd':
        config.data_files[config.data_files_count] =
          cag_option_get_value(&context);
        config.data_files_count++;
        break;

      case 'i':
        config.input_file = cag_option_get_value(&context);
        break;

      case 'q':
        config.query = cag_option_get_value(&context);
        break;

      case 'b':
        config.bundle = cag_option_get_value(&context);
        break;

      case 'f': {
        const char* format = cag_option_get_value(&context);
        if (strcmp(format, "binary"))
        {
          config.binary_format = true;
        }
        break;
      }

      case 'e':
        config.entrypoint = cag_option_get_value(&context);
        break;

      case 'h':
        printf("Usage: regoc [OPTION]...\n");
        printf("A C rego interpreter.\n\n");
        cag_option_print(options, CAG_ARRAY_SIZE(options), stdout);
        return EXIT_SUCCESS;
    }
  }

  if (config.query == NULL && config.mode != MODE_RUN)
  {
    if (context.index < argc)
    {
      config.query = argv[context.index];
    }

    if (config.query == NULL && config.entrypoint == NULL)
    {
      printf("No query provided.\n");
      return EXIT_FAILURE;
    }
  }

  rego = regoNew();

  if (config.mode == MODE_RUN)
  {
    if (config.binary_format)
    {
      bundle = regoBundleLoadBinary(rego, config.bundle);
    }
    else
    {
      bundle = regoBundleLoad(rego, config.bundle);
    }
  }
  else
  {
    for (data_index = 0; data_index < config.data_files_count; data_index++)
    {
      const char* file = config.data_files[data_index];
      if (is_json(file))
      {
        err = regoAddDataJSONFile(rego, file);
      }
      else
      {
        err = regoAddModuleFile(rego, file);
      }
      if (err == REGO_ERROR)
      {
        goto error;
      }
    }

    if (config.query != NULL)
    {
      regoSetQuery(rego, config.query);
    }

    if (config.entrypoint)
    {
      regoAddEntrypoint(rego, config.entrypoint);
    }

    bundle = regoBuild(rego);
  }

  if (!regoBundleOk(bundle))
  {
    goto error;
  }

  if (config.input_file != NULL)
  {
    err = regoSetInputJSONFile(rego, config.input_file);
    if (err == REGO_ERROR)
    {
      goto error;
    }
  }

  if (config.mode == MODE_BUILD)
  {
    if (config.binary_format)
    {
      err = regoBundleSaveBinary(rego, config.bundle, bundle);
    }
    else
    {
      err = regoBundleSave(rego, config.bundle, bundle);
    }

    if (err != REGO_OK)
    {
      goto error;
    }

    printf("Bundle built successfully\n");

    goto exit;
  }

  if (config.entrypoint)
  {
    output = regoBundleQueryEntrypoint(rego, bundle, config.entrypoint);
  }
  else
  {
    output = regoBundleQuery(rego, bundle);
  }

  if (!regoOutputOk(output))
  {
    goto error;
  }

  if (config.use_raw_query)
  {
    print_node(regoOutputNode(output), 0);
    goto exit;
  }
  else
  {
    print_output(output);
    goto exit;
  }

error:
  print_error(rego);
  rc = EXIT_FAILURE;

exit:
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
