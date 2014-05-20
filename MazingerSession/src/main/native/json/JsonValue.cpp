/*
 * JsonValue.cpp
 *
 *  Created on: 12/03/2014
 *      Author: bubu
 */

#include "JsonValue.h"
#include <stdio.h>

namespace json {

JsonValue::JsonValue() {
	// TODO Auto-generated constructor stub

}

JsonValue::~JsonValue() {
	// TODO Auto-generated destructor stub
}

const char* JsonValue::read(const char* str) {
	value.clear();
	str = skipSpaces(str);
	if (*str == '"')
	{
		bool backSlash = false;
		do
		{
			value.append (str, 1);
			if (backSlash)
				backSlash = false;
			else
				backSlash = (*str == '\\');
			str ++;
		} while (backSlash || *str != '"');
		if (*str)
		{
			value.append (str++, 1);
		}
	}
	else
	{
		while (*str > ' ' && *str != ',')
		{
			value.append (str, 1);
			str ++;
		}
	}

	return str;
}

void JsonValue::write(std::string& str, int indent) {
	str.append (value);
}


} /* namespace json */
