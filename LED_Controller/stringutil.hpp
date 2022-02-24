
/*
  Returns first non-whitespace character of string
*/
int getStringStart(const char *string, int str_len) {
  for (int i = 0; i < str_len; i++) if (string[i] != ' ' && string[i] != '\t') return i;
  return -1;
}

/*
  returns the position after the last character of substring in string if the substring is in string
  else -1 is returned 
*/
int startsWith(const char *string, int str_len, const char *substring) {
  int string_start = getStringStart(string, str_len);
  int sub_len = strlen(substring);
  int i = 0;
  for (; i < sub_len; i++) {
    if (string_start + i >= str_len) return -1;
    if (substring[i] != string[string_start + i]) return -1;
  }
  return i+1;
}

/*
  Returns the position of the first non-numeric character in string, -1 if no numeric character was found
*/
int endOfNumber(const char *string, int str_len) {
  int start = getStringStart(string, str_len);
  for (int i = start; i <= str_len; i++) if (string[i] < 48 || string[i] > 57) return i;
  return -1;
}


/*
    Returns number if found, terminates string after number
    the end int pointer number is increased by the index of the first non numeric.
    if no number is found, -1 is returned
*/
long getNumericArgument(char * input, int len, int *end) {
  int ns = endOfNumber(input, len);
  int start = getStringStart(input, len);
  if (ns-start <= 0) {
    Serial.print(F("NErr "));
    Serial.println(input);
    return -1;
  } 
  input[ns] = 0;
  *end += ns+1;
  return atol(input+start);
}
