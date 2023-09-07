#include <cargs.h>
#include <rego/rego_c.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DATA_FILES 128
#define MAX_VALUE_LENGTH 256

/**
 * This is the main configuration of all options available.
 */
static struct cag_option options[] = {
  {.identifier = 'l',
   .access_letters = "l",
   .access_name = "logging",
   .value_name = NULL,
   .description = "Enable logging"},

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
  bool logging_enabled;
  bool use_raw_query;
  const char* data_files[MAX_DATA_FILES];
  unsigned int data_files_count;
  const char* input_file;
  const char* query;
};

bool is_json(const char* file)
{
  size_t len;

  len = strlen(file);
  if (
    file[len - 1] != 'n' || file[len - 2] != 'o' || file[len - 3] != 's' ||
    file[len - 4] != 'j' || file[len - 5] != '.')
  {
    return false;
  }

  return true;
}

void print_node(regoNode* node, unsigned int indent)
{
  char value[MAX_VALUE_LENGTH];
  unsigned int i;
  unsigned int size;
  regoEnum type;

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
      regoNodeValue(node, value, MAX_VALUE_LENGTH);
      printf("(var %s)\n", value);
      return;

    case REGO_NODE_INT:
    case REGO_NODE_FLOAT:
      regoNodeValue(node, value, MAX_VALUE_LENGTH);
      printf("(number %s)\n", value);
      return;

    case REGO_NODE_STRING:
      regoNodeValue(node, value, MAX_VALUE_LENGTH);
      printf("(string %s)\n", value);
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
      printf("(%s", regoNodeTypeName(node));
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

int main(int argc, char** argv)
{
  printf("regoc %s\n", REGOCPP_VERSION);
  char identifier;
  cag_option_context context;
  struct regoc_configuration config = {
    .logging_enabled = false,
    .use_raw_query = false,
    .data_files_count = 0,
    .input_file = NULL,
    .query = NULL};
  unsigned int data_index;
  regoInterpreter* rego = NULL;
  regoResult* result = NULL;
  int rc = EXIT_SUCCESS;
  regoEnum err;

  cag_option_prepare(&context, options, CAG_ARRAY_SIZE(options), argc, argv);
  while (cag_option_fetch(&context))
  {
    identifier = cag_option_get(&context);
    switch (identifier)
    {
      case 'l':
        config.logging_enabled = true;
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

      case 'h':
        printf("Usage: regoc [OPTION]...\n");
        printf("A C rego interpreter.\n\n");
        cag_option_print(options, CAG_ARRAY_SIZE(options), stdout);
        return EXIT_SUCCESS;
    }
  }

  if (config.query == NULL)
  {
    if (context.index < argc)
    {
      config.query = argv[context.index];
    }

    if (config.query == NULL)
    {
      printf("No query provided.\n");
      return EXIT_FAILURE;
    }
  }

  rego = regoNew();
  if (config.logging_enabled)
  {
    regoSetLoggingEnabled(true);
  }

  regoSetExecutable(rego, argv[0]);

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

  if (config.input_file != NULL)
  {
    err = regoAddInputJSONFile(rego, config.input_file);
    if (err == REGO_ERROR)
    {
      goto error;
    }
  }

  result = regoQuery(rego, config.query);
  if (result == NULL)
  {
    goto error;
  }
  if (config.use_raw_query)
  {
    print_node(regoResultNode(result), 0);
    goto exit;
  }
  else
  {
    printf("%s\n", regoResultString(result));
    goto exit;
  }

error:
  printf("%s\n", regoGetError(rego));
  rc = EXIT_FAILURE;

exit:
  if (result != NULL)
  {
    regoFreeResult(result);
  }

  if (rego != NULL)
  {
    regoFree(rego);
  }

  return rc;
}
