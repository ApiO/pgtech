#include <cmath>
#include <regex>
#include <stdarg.h>

#include "runtime/array.h"
#include "runtime/json.h"
#include "runtime/temp_allocator.h"
#include "runtime/string_stream.h"
#include "runtime/string_pool.h"

namespace
{
  using namespace pge;
  using namespace json;

  const char *INVALID_TYPE    = "Invalid type: %s (expected %s)";
  const char *ENUM_MISMATCH   = "No enum match";
  const char *ANY_OF_MISSING  = "Data does not match any schemas from \"anyOf\"";
  const char *ONE_OF_MISSING  = "Data does not match any schemas from \"oneOf\"";
  const char *ONE_OF_MULTIPLE = "Data is valid against more than one schema from \"oneOf\": indices %d and %d";
  const char *NOT_PASSED      = "Data matches schema from \"not\"";
  // Numeric errors
  const char *NUMBER_MULTIPLE_OF       = "Value %f is not a multiple of %d";
  const char *NUMBER_MINIMUM           = "Value %f is less than minimum %f";
  const char *NUMBER_MINIMUM_EXCLUSIVE = "Value %f is equal to exclusive minimum %f";
  const char *NUMBER_MAXIMUM           = "Value %f is greater than maximum %f";
  const char *NUMBER_MAXIMUM_EXCLUSIVE = "Value %f is equal to exclusive maximum %f";
  // String errors
  const char *STRING_LENGTH_SHORT = "String is too short (%d chars), minimum %d";
  const char *STRING_LENGTH_LONG  = "String is too long (%d chars), maximum %d";
  const char *STRING_PATTERN      = "String does not match pattern: %s";
  // Object errors
  const char *OBJECT_PROPERTIES_MINIMUM    = "Too few properties defined (%d), minimum %d";
  const char *OBJECT_PROPERTIES_MAXIMUM    = "Too many properties defined (%d), maximum %d";
  const char *OBJECT_REQUIRED              = "Missing required property: %s";
  const char *OBJECT_ADDITIONAL_PROPERTIES = "Additional properties not allowed";
  const char *OBJECT_DEPENDENCY_KEY        = "Dependency failed - key must exist: %s (due to key: %s)";
  // Array errors
  const char *ARRAY_LENGTH_SHORT     = "Array is too short (%d), minimum %d";
  const char *ARRAY_LENGTH_LONG      = "Array is too long (%d), maximum %d";
  const char *ARRAY_UNIQUE           = "Array items are not unique (indices %d and %d)";
  const char *ARRAY_ADDITIONAL_ITEMS = "Additional items not allowed";
  // Format errors
  const char *FORMAT_CUSTOM    = "Format validation failed (%d)";
  const char *UNKNOWN_PROPERTY = "Unknown property (not in schema)";
  // type strings
  const char * OBJECT_TYPE_STRING = "object";
  const char *  ARRAY_TYPE_STRING = "array";
  const char * STRING_TYPE_STRING = "string";
  const char * NUMBER_TYPE_STRING = "number";
  const char *INTEGER_TYPE_STRING = "integer";
  const char *BOOLEAN_TYPE_STRING = "boolean";

  struct FindResult {
    u64 parent_id;
    i32 child_index;
  };

  FindResult find_parent(const Json &doc, u64 id)
  {
    FindResult fr;
    fr.parent_id   = NO_NODE;
    fr.child_index = 0;

    u64 prev = get_node(doc, id).prev;
    while (prev != NO_NODE) {
      const Json::Node &p = get_node(doc, prev);
      if (p.child == id) {
        fr.parent_id = prev;
        return fr;
      }
      ++fr.child_index;
      id   = p.id;
      prev = p.prev;
    }
    return fr;
  }

  u32 snprint_jpath(char *dest, u32 count, const Json &doc, u64 node)
  {
    const Json::Node &n = get_node(doc, node);

    if (n.prev == NO_NODE)
      return snprintf(dest, count, "#");

    const FindResult fr = find_parent(doc, node);
    bool  is_array_item = false;
    u32   inserted = 0;

    if (fr.parent_id != NO_NODE) {
      inserted = snprint_jpath(dest, count, doc, fr.parent_id);
      is_array_item = (get_type(doc, fr.parent_id) == JSON_ARRAY);
    }

    if (is_array_item)
      return inserted += snprintf(dest + inserted, count - inserted, "/%d", fr.child_index);

    return inserted += snprintf(dest + inserted, count - inserted, "/%s", n.name);
  }

  void push_validation_error(Json &doc, u64 node, const char *format, ...)
  {
    va_list args;
    char    tmp[1024];
    u32     offset = 0;

    offset += snprint_jpath(tmp + offset, 1024, doc, node);
    offset += sprintf(tmp + offset, ": ");
    va_start(args, format);
    offset += vsprintf(tmp + offset, format, args);
    va_end(args);
    array::push_back(doc._errors, string_pool::acquire(*doc._string_pool, tmp));
  }

   JsonType json_type_from_string(const char *str)
  {
    if (strcmp(str, OBJECT_TYPE_STRING) == 0) return JSON_OBJECT;
    else if (strcmp(str, ARRAY_TYPE_STRING) == 0) return JSON_ARRAY;
    else if (strcmp(str, STRING_TYPE_STRING) == 0) return JSON_STRING;
    else if (strcmp(str, NUMBER_TYPE_STRING) == 0) return JSON_NUMBER;
    else if (strcmp(str, INTEGER_TYPE_STRING) == 0) return JSON_INTEGER;
    else if (strcmp(str, BOOLEAN_TYPE_STRING) == 0) return JSON_BOOLEAN;

    return JSON_NULL;
  }

   const char *string_from_json_type(JsonType type)
  {
    if (type == JSON_OBJECT)  return OBJECT_TYPE_STRING;
    else if (type == JSON_ARRAY)   return ARRAY_TYPE_STRING;
    else if (type == JSON_STRING)  return STRING_TYPE_STRING;
    else if (type == JSON_NUMBER)  return NUMBER_TYPE_STRING;
    else if (type == JSON_INTEGER) return INTEGER_TYPE_STRING;
    else if (type == JSON_BOOLEAN) return BOOLEAN_TYPE_STRING;

    return NULL;
  }

   bool validate_generic(Json &data_doc, const Json &schema_doc, u64 data_id, u64 schema_id, bool report_errors);

   bool validate_enum(Json &data_doc, const Json &schema_doc, u64 data_id, u64 schema_id, bool report_errors)
  {
    if (!has(schema_doc, schema_id, "enum"))
      return true;

    const Json::Node &data_node = get_node(data_doc, data_id);

    // the schema type node is an array
    u64 next = get_node(schema_doc, schema_id, "enum").child;
    while (next != NO_NODE) {
      const Json::Node &enum_node = get_node(schema_doc, next);
      if (data_node.type == enum_node.type) {
        if ((data_node.type == JSON_STRING) ?
            strcmp(data_node.value.string, enum_node.value.string) == 0 :
            data_node.value.raw == enum_node.value.raw) return true;
      }
      next = enum_node.next;
    }

    if (report_errors) push_validation_error(data_doc, data_id, ENUM_MISMATCH);
    return false;
  }

   bool validate_type(Json &data_doc, const Json &schema_doc, u64 data_id, u64 schema_id, bool report_errors)
  {
    if (!has(schema_doc, schema_id, "type"))
      return true;

    const JsonType    data_type        = get_type(data_doc, data_id);
    const char       *data_type_string = string_from_json_type(data_type);
    const Json::Node &type_node        = get_node(schema_doc, schema_id, "type");

    if (type_node.type == JSON_STRING) {
      // the schema type node is a string
      const JsonType schema_type = json_type_from_string(type_node.value.string);
      if (strcmp(type_node.value.string, "any") == 0)              return true;
      if (schema_type == JSON_NUMBER && data_type == JSON_INTEGER) return true;
      if (schema_type == data_type)                                return true;

      if (report_errors) push_validation_error(data_doc, data_id, INVALID_TYPE, data_type_string, type_node.value.string);
      return false;
    }

    {
      using namespace string_stream;
      TempAllocator512 ta(*schema_doc._nodes._data._allocator);
      Buffer allowed_types(ta);

      // the schema type node is an array
      u64 next = type_node.child;
      while (next != NO_NODE) {
        const char    *type_str    = get_node(schema_doc, next).value.string;
        const JsonType schema_type = json_type_from_string(type_str);

        if (strcmp(type_str, "any") == 0)                            return true;
        if (schema_type == JSON_NUMBER && data_type == JSON_INTEGER) return true;
        if (schema_type == data_type)                                return true;

        allowed_types << type_str << "/";
        next = get_node(schema_doc, next).next;
      }
      // remove trailing slash
      array::resize(allowed_types, array::size(allowed_types) - 1);
      if (report_errors) push_validation_error(data_doc, data_id, INVALID_TYPE, data_type_string, allowed_types);
    }

    return false;
  }

   bool validate_numeric(Json &data_doc, const Json &schema_doc, u64 data_id, u64 schema_id, bool report_errors)
  {
    const Json::Node &numeric = get_node(data_doc, data_id);
    const f64 value = numeric.type == JSON_INTEGER ? (f64)numeric.value.integer : numeric.value.number;
    bool valid = true;

    if (has(schema_doc, schema_id, "minimum")) {
      const bool exclusive = get_bool(schema_doc, schema_id, "exclusiveMinimum", false);
      const f64  minimum   = get_number(schema_doc, schema_id, "minimum");
      if (value < minimum) {
        valid = false;
        if (report_errors) push_validation_error(data_doc, data_id, NUMBER_MINIMUM, value, minimum);
      } else if (exclusive && value == minimum) {
        valid = false;
        if (report_errors) push_validation_error(data_doc, data_id, NUMBER_MINIMUM_EXCLUSIVE, value, minimum);
      }
    }

    if (has(schema_doc, schema_id, "maximum")) {
      const bool exclusive = get_bool(schema_doc, schema_id, "exclusiveMaximum", false);
      const f64  maximum   = get_number(schema_doc, schema_id, "maximum");
      if (value > maximum) {
        valid = false;
        if (report_errors) push_validation_error(data_doc, data_id, NUMBER_MAXIMUM_EXCLUSIVE, value, maximum);
      } else if (exclusive && value == maximum) {
        valid = false;
        if (report_errors) push_validation_error(data_doc, data_id, NUMBER_MAXIMUM_EXCLUSIVE, value, maximum);
      }
    }

    if (has(schema_doc, schema_id, "divisibleBy")) {
      const f64 multiple = get_number(schema_doc, schema_id, "divisibleBy");
      if (fmod(value, multiple) != 0.0) {
        valid = false;
        if (report_errors) push_validation_error(data_doc, data_id, NUMBER_MULTIPLE_OF, value, multiple);
      }
    }

    return valid;
  }

   bool validate_string(Json &data_doc, const Json &schema_doc, u64 data_id, u64 schema_id, bool report_errors)
  {
    const char *value  = get_node(data_doc, data_id).value.string;
    const u32   length = strlen(value);

    bool valid = true;

    if (has(schema_doc, schema_id, "minLength")) {
      const u32 min_length = (u32)get_integer(schema_doc, schema_id, "minLength");
      if (length < min_length) {
        valid = false;
        if (report_errors) push_validation_error(data_doc, data_id, STRING_LENGTH_SHORT, length, min_length);
      }
    }

    if (has(schema_doc, schema_id, "maxLength")) {
      const u32 max_length = (u32)get_integer(schema_doc, schema_id, "maxLength");
      if (length > max_length) {
        valid = false;
        if (report_errors) push_validation_error(data_doc, data_id, STRING_LENGTH_LONG, length, max_length);
      }
    }

    if (has(schema_doc, schema_id, "pattern")) {
      const char *pattern = get_string(schema_doc, schema_id, "pattern");
      std::regex reg(pattern);
      if (!regex_match(value, reg)) {
        valid = false;
        if (report_errors) push_validation_error(data_doc, data_id, STRING_PATTERN, pattern);
      }
    }

    return valid;
  }

   bool recursive_compare(Json &data_doc, u64 a, u64 b)
  {
    const Json::Node &na = get_node(data_doc, a);
    const Json::Node &nb = get_node(data_doc, b);

    if (na.type != nb.type)
      return false;

    if (na.type != JSON_OBJECT && na.type != JSON_ARRAY)
      return na.value.raw == nb.value.raw;

    if (size(data_doc, a) != size(data_doc, b))
      return false;

    if (na.type == JSON_ARRAY) {
      u64 next_a = na.child, next_b = nb.child;
      while (next_a != NO_NODE) {
        if (!recursive_compare(data_doc, next_a, next_b))
          return false;
        next_a = get_node(data_doc, next_a).next;
        next_b = get_node(data_doc, next_b).next;
      }
    } else {
      u64 next = na.child;
      while (next != NO_NODE) {
        if (!recursive_compare(data_doc, next, get_id(data_doc, b, get_node(data_doc, next).name)))
          return false;
        next = get_node(data_doc, next).next;
      }
    }
    return true;
  }

   bool validate_array(Json &data_doc, const Json &schema_doc, u64 data_id, u64 schema_id, bool report_errors)
  {
    bool valid = true;

    const i32 num_items = size(data_doc, data_id);

    // check that the array has more properties than minProperties
    if (has(schema_doc, schema_id, "minItems")) {
      const i32 min_items = get_integer(schema_doc, schema_id, "minItems");
      if (num_items < min_items) {
        valid = false;
        if (report_errors) push_validation_error(data_doc, data_id, ARRAY_LENGTH_SHORT, num_items, min_items);
      }
    }

    // check that the array has less properties than maxProperties
    if (has(schema_doc, schema_id, "maxItems")) {
      const i32 max_items = get_integer(schema_doc, schema_id, "maxItems");
      if (num_items > max_items) {
        valid = false;
        if (report_errors) push_validation_error(data_doc, data_id, ARRAY_LENGTH_LONG, num_items, max_items);
      }
    }

    // check that the array items items are unique
    if (has(schema_doc, schema_id, "uniqueItems") && get_bool(schema_doc, schema_id, "uniqueItems")) {
      for (i32 i = 0; i < num_items; i++) {
        for (i32 j = i + 1; j < num_items; j++) {
          if (recursive_compare(data_doc, get_id(data_doc, data_id, i), get_id(data_doc, data_id, j))) {
            valid = false;
            if (report_errors) push_validation_error(data_doc, data_id, ARRAY_UNIQUE, i, j);
          }
        }
      }
    }

    // if there is no item schema defined, and no additionalProperties attribute, we're done
    if (!has(schema_doc, schema_id, "items"))
      return valid;

    const u64 items_schema = get_id(schema_doc, schema_id, "items");

    // if "items" is an object : simply validate each item with itre
    if (get_type(schema_doc, items_schema) == JSON_OBJECT) {
      for (i32 i = 0; i < num_items; i++)
        valid &= validate_generic(data_doc, schema_doc, get_id(data_doc, data_id, i), items_schema, report_errors);
      return valid; // we're done
    }

    // "items" is an array: it's a tuple
    const i32 tuple_size = size(schema_doc, items_schema);
    for (i32 i = 0; i < tuple_size; i++)
      valid &= validate_generic(data_doc, schema_doc, get_id(data_doc, data_id, i), get_id(schema_doc, items_schema, i), report_errors);

    // if there is no "additionalItems" property in the schema, wwe're done
    if (!has(schema_doc, schema_id, "additionalItems"))
      return valid;

    const Json::Node &additional_items = get_node(schema_doc, schema_id, "additionalItems");

    if (additional_items.type == JSON_BOOLEAN) {
      // if additional items is setted to true, we're done
      if (additional_items.value.boolean == true)
        return valid;
      // else, we have to check if there is no additional items
      if (num_items > tuple_size) {
        if (report_errors) push_validation_error(data_doc, data_id, ARRAY_ADDITIONAL_ITEMS);
        return false;
      }
    }

    // additional items is a schema: we have to check every additional items with it
    for (i32 i = tuple_size; i < num_items; i++)
      valid &= validate_generic(data_doc, schema_doc, get_id(data_doc, data_id, i), additional_items.id, report_errors);

    return valid;
  }

   u64 get_pattern_prop_schema(const char *prop_name, const Json &schema_doc, u64 patternProperties)
  {
    u64 next = get_node(schema_doc, patternProperties).child;

    while (next != NO_NODE) {
      const Json::Node &schema = get_node(schema_doc, next);
      const std::regex e(schema.name);

      if (regex_match(prop_name, e))
        return schema.id;
      next = schema.next;
    }
    return NO_NODE;
  }

   bool validate_object(Json &data_doc, const Json &schema_doc, u64 data_id, u64 schema_id, bool report_errors)
  {
    bool valid = true;

    const i32 num_properties = size(data_doc, data_id);
    // check that the object has more properties than minProperties
    if (has(schema_doc, schema_id, "minProperties")) {
      const i32 min_properties = get_integer(schema_doc, schema_id, "minProperties");
      if (num_properties < min_properties) {
        valid = false;
        if (report_errors) push_validation_error(data_doc, data_id, OBJECT_PROPERTIES_MINIMUM, num_properties, min_properties);
      }
    }

    // check that the object has less properties than maxProperties
    if (has(schema_doc, schema_id, "maxProperties")) {
      const i32 max_properties = get_integer(schema_doc, schema_id, "maxProperties");
      if (num_properties > max_properties) {
        valid = false;
        if (report_errors) push_validation_error(data_doc, data_id, OBJECT_PROPERTIES_MAXIMUM, num_properties, max_properties);
      }
    }

    // check that all required properties are defined
    if (has(schema_doc, schema_id, "required")) {
      const u64 requires     = get_id(schema_doc, schema_id, "required");
      const i32 num_required = size(schema_doc, requires);

      for (i32 i = 0; i < num_required; i++) {
        const char *required_name = get_string(schema_doc, requires, i);
        if (!has(data_doc, data_id, required_name)) {
          valid = false;
          if (report_errors) push_validation_error(data_doc, data_id, OBJECT_REQUIRED, required_name);
        }
      }
    }

    // setting up the properties validation
    const u64 prop_schemas    = has(schema_doc, schema_id, "properties") ? get_id(schema_doc, schema_id, "properties") : NO_NODE;
    const u64 pattern_schemas = has(schema_doc, schema_id, "patternProperties") ? get_id(schema_doc, schema_id, "patternProperties") : NO_NODE;

    u64  additional_schema = NO_NODE;
    bool additional_bool   = true;

    if (has(schema_doc, schema_id, "additionalProperties")) {
      const Json::Node &additional_props_node = get_node(schema_doc, schema_id, "additionalProperties");
      if (additional_props_node.type == JSON_OBJECT)
        additional_schema = additional_props_node.id;
      else
        additional_bool = additional_props_node.value.boolean;
    }

    // if there is no property schema defined, and no additionalProperties attribute, we're done
    if (prop_schemas == NO_NODE && pattern_schemas == NO_NODE  && additional_schema == NO_NODE && additional_bool)
      return valid;

    u64 pattern_schema = NO_NODE;
    u64 next = get_node(data_doc, data_id).child;

    while (next != NO_NODE) {
      const Json::Node &prop = get_node(data_doc, next);
      if (prop_schemas != NO_NODE && has(schema_doc, prop_schemas, prop.name)) {
        // there is a schema defined for this property: use it to validate the property
        valid &= validate_generic(data_doc, schema_doc, prop.id, get_id(schema_doc, prop_schemas, prop.name), report_errors);
      } else if (pattern_schemas != NO_NODE && (pattern_schema = get_pattern_prop_schema(prop.name, schema_doc, pattern_schemas), pattern_schema != NO_NODE)) {
        // the property name does match a pattern : validate with the pattern schema
        valid &= validate_generic(data_doc, schema_doc, prop.id, pattern_schema, report_errors);
      } else if (additional_schema != NO_NODE) {
        // additionalProperties is a schema : use this one to validate the property
        valid &= validate_generic(data_doc, schema_doc, prop.id, additional_schema, report_errors);
      } else if (!additional_bool) {
        // additionalProperties is a boolean and setted to false : the property sould not exist
        valid = false;
        if (report_errors) push_validation_error(data_doc, prop.id, OBJECT_ADDITIONAL_PROPERTIES);
      }
      next = prop.next;
    }

    // check dependencies
    if (!has(schema_doc, schema_id, "dependencies"))
      return valid;

    next = get_node(schema_doc, schema_id, "dependencies").child;
    while (next != NO_NODE) {
      const Json::Node &dependency = get_node(schema_doc, next);

      if (!has(data_doc, data_id, dependency.name)) {
        next = dependency.next;
        continue;
      }

      if (dependency.type == JSON_ARRAY) {
        // propery dependency
        const i32 num_properties = size(schema_doc, dependency.id);
        for (i32 i = 0; i < num_properties; i++) {
          const char *property_name = get_string(schema_doc, dependency.id, i);
          if (!has(data_doc, data_id, property_name)) {
            valid = false;
            if (report_errors) push_validation_error(data_doc, data_id, OBJECT_DEPENDENCY_KEY, property_name, dependency.name);
          }
        }
      } else {
        // schema dependency
        valid &= validate_generic(data_doc, schema_doc, data_id, dependency.id, report_errors);
      }
      next = dependency.next;
    }
    return valid;
  }

   bool validate_all_of(Json &data_doc, const Json &schema_doc, u64 data_id, u64 schema_id, bool report_errors)
  {
    if (!has(schema_doc, schema_id, "allOf"))
      return true;
    schema_id = get_id(schema_doc, schema_id, "allOf");

    const i32 num_schemas = size(schema_doc, schema_id);
    bool valid = true;
    for (i32 i = 0; i < num_schemas; i++)
      valid &= validate_generic(data_doc, schema_doc, data_id, schema_id, report_errors);
    return valid;
  }

   bool validate_any_of(Json &data_doc, const Json &schema_doc, u64 data_id, u64 schema_id, bool report_errors)
  {
    if (!has(schema_doc, schema_id, "anyOf"))
      return true;
    schema_id = get_id(schema_doc, schema_id, "anyOf");

    const i32 num_schemas = size(schema_doc, schema_id);
    for (i32 i = 0; i < num_schemas; i++) {
      if (validate_generic(data_doc, schema_doc, data_id, get_id(schema_doc, schema_id, i), false))
        return true;
    }
    if (report_errors) push_validation_error(data_doc, data_id, ANY_OF_MISSING);
    return false;
  }

   bool validate_one_of(Json &data_doc, const Json &schema_doc, u64 data_id, u64 schema_id, bool report_errors)
  {
    if (!has(schema_doc, schema_id, "oneOf"))
      return true;
    schema_id = get_id(schema_doc, schema_id, "oneOf");

    const i32 num_schemas = size(schema_doc, schema_id);
    i32 first_valid = -1;
    i32 num_valids  = 0;
    for (i32 i = 0; i < num_schemas; i++) {
      if (validate_generic(data_doc, schema_doc, data_id, get_id(schema_doc, schema_id, i), false)) {
        if (num_valids == 0) first_valid = i;
        if (++num_valids >= 2) {
          if (report_errors) push_validation_error(data_doc, data_id, ONE_OF_MULTIPLE, first_valid, i);
          return false;
        }
      }
    }

    if (num_valids == 0) {
      if (report_errors) push_validation_error(data_doc, data_id, ONE_OF_MISSING);
      return false;
    }

    return true;
  }

   bool validate_not(Json &data_doc, const Json &schema_doc, u64 data_id, u64 schema_id, bool report_errors)
  {
    if (!has(schema_doc, schema_id, "not"))
      return true;
    schema_id = get_id(schema_doc, schema_id, "not");

    bool passed = validate_generic(data_doc, schema_doc, data_id, schema_id, false);
    if (passed && report_errors)
      push_validation_error(data_doc, data_id, NOT_PASSED);

    return !passed;
  }

   void add_default_properties(Json &data_doc, const Json &schema_doc, u64 data_id, u64 schema_id)
  {
    if (!has(schema_doc, schema_id, "properties"))
      return;

    u64 next = get_node(schema_doc, schema_id, "properties").child;
    while (next != NO_NODE) {
      const Json::Node &prop = get_node(schema_doc, next);
      if (has(schema_doc, prop.id, "default") && !has(data_doc, data_id, prop.name)) {
        copy(data_doc, schema_doc, data_id, get_id(schema_doc, prop.id, "default"), prop.name);
      }
      next = prop.next;
    }
  }

   bool validate_generic(Json &data_doc, const Json &schema_doc, u64 data_id, u64 schema_id, bool report_errors)
  {
    bool valid = true;

    valid &= validate_enum(data_doc, schema_doc, data_id, schema_id, report_errors);
    valid &= validate_type(data_doc, schema_doc, data_id, schema_id, report_errors);

    switch (get_type(data_doc, data_id)) {
    case JSON_OBJECT:
      valid &= validate_object(data_doc, schema_doc, data_id, schema_id, report_errors);
      add_default_properties(data_doc, schema_doc, data_id, schema_id);
      break;
    case JSON_ARRAY:
      valid &= validate_array(data_doc, schema_doc, data_id, schema_id, report_errors);
      break;
    case JSON_INTEGER:
    case JSON_NUMBER:
      valid &= validate_numeric(data_doc, schema_doc, data_id, schema_id, report_errors);
      break;
    case JSON_STRING:
      valid &= validate_string(data_doc, schema_doc, data_id, schema_id, report_errors);
      break;
    }

    valid &= validate_all_of(data_doc, schema_doc, data_id, schema_id, report_errors);
    valid &= validate_any_of(data_doc, schema_doc, data_id, schema_id, report_errors);
    valid &= validate_one_of(data_doc, schema_doc, data_id, schema_id, report_errors);
    valid &= validate_not(data_doc, schema_doc, data_id, schema_id, report_errors);

    return valid;
  }
}

namespace pge
{
  namespace json
  {
    bool validate(Json &data_doc, const Json &schema_doc, u64 data_node, u64 schema_node)
    {
      if (data_node == NO_NODE) data_node   = data_doc._root;
      if (schema_node == NO_NODE) schema_node = schema_doc._root;
      return validate_generic(data_doc, schema_doc, data_node, schema_node, true);
    }
  }
}