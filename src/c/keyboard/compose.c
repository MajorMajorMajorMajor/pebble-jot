#include "keyboard/compose.h"
#include <string.h>

// Source: original MessagEase Compose reference supplied in #32.
// Duplicate pair entries retain the first listed meaning; later variations
// remain available by cycling. The reference contains occasionally missing
// spaces/ambiguous typography; normalize only the pair syntax, not glyphs.
// 222 reference entries; 217 distinct pairs; 6 ambiguities.
typedef struct { const char *left, *right, *result; } ComposePair;
static const ComposePair s_pairs[] = {
  {"f", "/", "ƒ"},
  {",", ",", "\""},
  {".", ".", "…"},
  {"+", "|", "†"},
  {"+", "+", "‡"},
  {"%", "%", "‰"},
  {"S", "^", "Š"},
  {"O", "E", "Œ"},
  {"<", "'", "'"},
  {">", "'", "'"},
  {"`", "`", "\""},
  {"´", "´", "\""},
  {"n", "-", "–"},
  {"m", "-", "—"},
  {"~", "-", "˜"},
  {"T", "M", "™"},
  {"s", "^", "š"},
  {"o", "e", "œ"},
  {"Y", "¨", "Ÿ"},
  {"!", "/", "¡"},
  {"c", "/", "¢"},
  {"L", "-", "£"},
  {"o", "=", "¤"},
  {"Y", "=", "¥"},
  {"|", "-", "¦"},
  {"S", "/", "§"},
  {"c", "o", "©"},
  {"<", "<", "«"},
  {"-", "|", "¬"},
  {"o", "r", "®"},
  {"-", "^", "¯"},
  {"+", "-", "±"},
  {"-", "+", "∓"},
  {"o", ">", "♂"},
  {"o", "+", "♀"},
  {"2", "^", "²"},
  {"3", "^", "³"},
  {"m", "u", "µ"},
  {"p", "p", "¶"},
  {"1", "^", "¹"},
  {">", ">", "»"},
  {"1", "4", "¼"},
  {"1", "2", "½"},
  {"3", "4", "¾"},
  {"?", "/", "¿"},
  {"A", "`", "À"},
  {"A", "´", "Á"},
  {"A", "^", "Â"},
  {"A", "~", "Ã"},
  {"A", "¨", "Ä"},
  {"A", "°", "Å"},
  {"A", "E", "Æ"},
  {"A", "C", "Ą"},
  {"C", "C", "Ç"},
  {"C", "^", "Č"},
  {"C", "´", "Ć"},
  {"D", "-", "Ð"},
  {"D", "^", "Ď"},
  {"E", "`", "È"},
  {"E", "´", "É"},
  {"E", "^", "Ê"},
  {"E", "¨", "Ë"},
  {"E", "C", "Ę"},
  {"E", ".", "Ė"},
  {"I", "`", "Ì"},
  {"I", "´", "Í"},
  {"I", "^", "Î"},
  {"I", "¨", "Ï"},
  {"I", "C", "Į"},
  {"I", ".", "İ"},
  {"i", ".", "ı"},
  {"G", "~", "Ğ"},
  {"g", "~", "ğ"},
  {"N", "^", "Ň"},
  {"N", "~", "Ñ"},
  {"O", "`", "Ò"},
  {"O", "´", "Ó"},
  {"O", "^", "Ô"},
  {"O", "~", "Õ"},
  {"O", "¨", "Ö"},
  {"O", "\"", "Ő"},
  {"x", "x", "×"},
  {"O", "/", "Ø"},
  {"R", "^", "Ř"},
  {"T", "^", "Ť"},
  {"U", "`", "Ù"},
  {"U", "´", "Ú"},
  {"U", "^", "Û"},
  {"U", "¨", "Ü"},
  {"U", "°", "Ů"},
  {"U", "\"", "Ű"},
  {"U", "C", "Ų"},
  {"U", "-", "Ū"},
  {"U", "U", "Ŭ"},
  {"U", "u", "Ŭ"},
  {"Y", "´", "Ý"},
  {"Z", "^", "Ž"},
  {"I", "P", "Þ"},
  {"s", "s", "ß"},
  {"a", "`", "à"},
  {"a", "´", "á"},
  {"a", "^", "â"},
  {"a", "~", "ã"},
  {"a", "¨", "ä"},
  {"a", "°", "å"},
  {"a", "e", "æ"},
  {"a", "c", "ą"},
  {"c", "c", "ç"},
  {"c", "^", "č"},
  {"c", "´", "ć"},
  {"d", "-", "ð"},
  {"d", "^", "ď"},
  {"e", "`", "è"},
  {"e", "´", "é"},
  {"e", "^", "ê"},
  {"e", "¨", "ë"},
  {"e", "c", "ę"},
  {"e", ".", "ė"},
  {"i", "`", "ì"},
  {"i", "´", "í"},
  {"i", "^", "î"},
  {"i", "¨", "ï"},
  {"i", "c", "į"},
  {"n", "^", "ň"},
  {"n", "~", "ñ"},
  {"o", "`", "ò"},
  {"o", "´", "ó"},
  {"o", "^", "ô"},
  {"o", "~", "õ"},
  {"o", "\"", "ő"},
  {"o", "¨", "ö"},
  {"-", ":", "÷"},
  {"o", "/", "ø"},
  {"r", "^", "ř"},
  {"s", "ˇ", "š"},
  {"S", "ˇ", "Š"},
  {"s", ">", "ş"},
  {"S", ">", "Ş"},
  {"t", ">", "ţ"},
  {"T", ">", "Ţ"},
  {"t", "^", "ť"},
  {"u", "`", "ù"},
  {"u", "´", "ú"},
  {"u", "^", "û"},
  {"u", "¨", "ü"},
  {"u", "°", "ů"},
  {"u", "\"", "ű"},
  {"u", "c", "ų"},
  {"u", "-", "ū"},
  {"u", "U", "ŭ"},
  {"u", "u", "ŭ"},
  {"y", "´", "ý"},
  {"z", "^", "ž"},
  {"i", "p", "þ"},
  {"y", "¨", "ÿ"},
  {"A", "L", "Α"},
  {"B", "E", "Β"},
  {"G", "A", "Γ"},
  {"D", "E", "Δ"},
  {"E", "P", "Ε"},
  {"Z", "E", "Ζ"},
  {"E", "T", "Η"},
  {"T", "E", "Θ"},
  {"I", "O", "Ι"},
  {"K", "A", "Κ"},
  {"L", "A", "Λ"},
  {"M", "U", "Μ"},
  {"N", "U", "Ν"},
  {"X", "I", "Ξ"},
  {"O", "N", "Ο"},
  {"P", "I", "Π"},
  {"R", "O", "Ρ"},
  {"S", "I", "Σ"},
  {"T", "A", "Τ"},
  {"U", "P", "Υ"},
  {"P", "H", "Φ"},
  {"C", "H", "Χ"},
  {"P", "S", "Ψ"},
  {"O", "M", "Ω"},
  {"a", "l", "α"},
  {"b", "e", "β"},
  {"g", "a", "γ"},
  {"d", "e", "δ"},
  {"e", "p", "ε"},
  {"z", "e", "ζ"},
  {"e", "t", "η"},
  {"t", "e", "θ"},
  {"i", "o", "ι"},
  {"k", "a", "κ"},
  {"l", "a", "λ"},
  {"n", "u", "ν"},
  {"x", "i", "ξ"},
  {"o", "n", "ο"},
  {"p", "i", "π"},
  {"r", "o", "ρ"},
  {"f", "s", "ς"},
  {"s", "i", "σ"},
  {"t", "a", "τ"},
  {"u", "p", "υ"},
  {"p", "h", "φ"},
  {"c", "h", "χ"},
  {"p", "s", "ψ"},
  {"o", "m", "ω"},
  {"o", "h", "Ω"},
  {"p", "d", "∂"},
  {"s", "q", "√"},
  {"r", "2", "√"},
  {"o", "o", "∞"},
  {"s", "\\", "∫"},
  {"~", "=", "≈"},
  {"/", "=", "≠"},
  {"<", "=", "≤"},
  {">", "=", "≥"},
  {"f", "f", "₣"},
  {"l", "r", "₤"},
  {"p", "z", "₧"},
  {"!", "?", "‽"},
};

// Historical s cycle requested by the Jot user explicitly. This order is
// deliberate and independent of conflicting s+^ descriptions in the table.
static const char *const s_s_cycle[] = {"s", "š", "ş", "ŝ", "ß", "∫"};
static const char *const s_cap_s_cycle[] = {"S", "Š", "Ş", "Ŝ"};

size_t jot_compose_pair_count(void) {
  return sizeof s_pairs / sizeof s_pairs[0];
}

// Walk back to the start of one UTF-8 codepoint from a valid byte boundary.
// A note's stored cursor is guaranteed to be at such a boundary.
static size_t prv_previous(const char *text, size_t cursor) {
  if (cursor == 0) return 0;
  --cursor;
  while (cursor > 0 && (((unsigned char)text[cursor] & 0xC0) == 0x80))
    --cursor;
  return cursor;
}

static bool prv_matches(const char *bytes, size_t len, const char *glyph) {
  return len == strlen(glyph) && memcmp(bytes, glyph, len) == 0;
}

static const char *prv_explicit_cycle(const char *bytes, size_t len) {
  for (size_t i = 0; i < sizeof s_s_cycle / sizeof s_s_cycle[0]; ++i) {
    if (prv_matches(bytes, len, s_s_cycle[i]))
      return s_s_cycle[(i + 1) % (sizeof s_s_cycle / sizeof s_s_cycle[0])];
  }
  for (size_t i = 0;
       i < sizeof s_cap_s_cycle / sizeof s_cap_s_cycle[0]; ++i) {
    if (prv_matches(bytes, len, s_cap_s_cycle[i]))
      return s_cap_s_cycle[(i + 1) %
          (sizeof s_cap_s_cycle / sizeof s_cap_s_cycle[0])];
  }
  return NULL;
}

static const char *prv_cycle(const char *bytes, size_t len) {
  const char *special = prv_explicit_cycle(bytes, len);
  if (special) return special;

  // Determine which base character owns the original or a composed result.
  // This permits cycling a character that was created via direct input, too.
  const char *base = NULL;
  const char *current_result = NULL;
  for (size_t i = 0; i < jot_compose_pair_count(); ++i) {
    const ComposePair *pair = &s_pairs[i];
    if (prv_matches(bytes, len, pair->left)) {
      base = pair->left;
      break;
    }
  }
  if (!base) {
    for (size_t i = 0; i < jot_compose_pair_count(); ++i) {
      const ComposePair *pair = &s_pairs[i];
      if (prv_matches(bytes, len, pair->result)) {
        base = pair->left;
        current_result = pair->result;
        break;
      }
    }
  }
  if (!base) return NULL;

  bool found_current = current_result == NULL;
  // Iterate in reference order, skipping non-unique variants and
  // no-op mappings. Wrap after the last variant back to the base.
  for (size_t i = 0; i < jot_compose_pair_count(); ++i) {
    const ComposePair *pair = &s_pairs[i];
    if (strcmp(pair->left, base) != 0 ||
        strcmp(pair->result, base) == 0) continue;
    if (!found_current) {
      if (strcmp(pair->result, current_result) == 0) found_current = true;
      continue;
    }
    if (!prv_matches(bytes, len, pair->result))
      return pair->result;
  }
  return base;
}

bool jot_compose_at(const char *text, size_t cursor, JotComposeEdit *out) {
  if (!out) return false;
  *out = (JotComposeEdit){0};
  if (!text) return false;
  const size_t len = strlen(text);
  if (cursor == 0 || cursor > len) return false;
  if (cursor < len && ((unsigned char)text[cursor] & 0xC0) == 0x80)
    return false;

  const size_t prev = prv_previous(text, cursor);
  const size_t prev_len = cursor - prev;
  // Pair conversion takes precedence over cycling a single character.
  if (prev > 0) {
    const size_t first = prv_previous(text, prev);
    for (size_t i = 0; i < jot_compose_pair_count(); ++i) {
      const ComposePair *pair = &s_pairs[i];
      if (prv_matches(text + first, prev - first, pair->left) &&
          prv_matches(text + prev, prev_len, pair->right)) {
        out->codepoints_before_cursor = 2;
        out->replacement = pair->result;
        return true;
      }
    }
  }

  const char *replacement = prv_cycle(text + prev, prev_len);
  if (!replacement || prv_matches(text + prev, prev_len, replacement))
    return false;
  out->codepoints_before_cursor = 1;
  out->replacement = replacement;
  return true;
}
