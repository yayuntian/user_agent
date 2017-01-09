#include <nmmintrin.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extractor.h"

#define MAX_INTERESTED_PAIRS 8
#define MAX_ENRICHEE 32
#define MAX_ORIG_NAME_LEN 128
#define MAX_ORIG_VALUE_LEN 1024
#define MAX_ENRICHED_VALUE_LEN 4096

#if __GNUC__ >= 3
    #define likely(x) __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define likely(x) (x)
    #define unlikely(x) (x)
#endif

struct interested_pair {
    int name_len;
    char *name;
    enricher enricher__;
};
static struct interested_pair interested_pairs[MAX_INTERESTED_PAIRS];
static int num_interested_pairs = 0;

static struct enrichee enrichees[MAX_ENRICHEE];

int init()
{
    for (int i = 0; i < MAX_ENRICHEE; i++) {
        enrichees[i].enriched_value = (char *)malloc(MAX_ENRICHED_VALUE_LEN);
    }

    return 0;
}

int register_enricher(const char *interested_name, enricher enricher__)
{
    int name_len = strlen(interested_name);
    interested_pairs[num_interested_pairs].name_len = name_len; // not include trailing \0
    interested_pairs[num_interested_pairs].name = (char *)malloc(name_len + 1);
    strncpy(interested_pairs[num_interested_pairs].name, interested_name, name_len + 1); // include trailing \0
    interested_pairs[num_interested_pairs].enricher__ = enricher__;
    num_interested_pairs++;
    return 0;
}

// 当找到感兴趣的字符后返回，或者整个字符串都查找完没有发现感兴趣的字符
int find_delimiter_sse42(const char *str2scan, const char *delimiters, size_t set_size, int *found)
{
    *found = 0;

    __m128i __m_delimiters = _mm_loadu_si128((const __m128i *)delimiters);
    __m128i __m_str2scan = _mm_loadu_si128((void *)str2scan);
    int r = _mm_cmpestri(__m_delimiters, set_size, __m_str2scan, 16, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_LEAST_SIGNIFICANT);
    if (likely(r != 16)) {
        *found = 1;
    }

    return r;
}

int is_pair_interested(const char *string_start, int string_len, struct interested_pair **interested)
{
    int is_interested = 0;

    for (int i = 0; i < num_interested_pairs; i++) {
        /* 先比一下字符串长度，可以节省一些CPU开销 */
        if ((interested_pairs[i].name_len == string_len) && (0 == strncmp(interested_pairs[i].name, string_start + 1, string_len))) {
            *interested = &interested_pairs[i];
            is_interested = 1;
            //printf("Found interested name: %.*s\n", string_len, string_start + 1);
            break; // Only one enricher per pair.
        }
    }

    return is_interested;
}

// 本函数会extract enricher感兴趣的name/value，调用该enricher的enrich函数。会将insertion数据结构中保存新字符串的buf的指针传入，这样避免enricher自己去申请内存
// 函数执行完后获得了所有insertion，包括个数，原始串中要被替换的字符串的起始和结束位置，用来替换的字符串及长度。这些信息将被传给生成最终结果的函数
int extract(const char *buf, const char *buf_end)
{
    const char delimiters[] __attribute__((aligned (16))) = "\",:{}";
    int found;
    const char *pos = buf;
    const char *prev_delimiter = NULL;
    int num_enrichees = 0;
    const char *string_start = NULL;
    const char *string_end = NULL;
    int string_len;
    int is_in_string = 0;
    int is_interested = 0;
    int prefix_space;
    int suffix_space;
    int consecutive_backslash_num;
    struct interested_pair *interested = NULL;

    while (pos < buf_end) {
        if ((buf_end - pos) >= 16) { // 剩余小于16字节的数据无法用此函数比较
            int r = find_delimiter_sse42(pos, delimiters, sizeof(delimiters) - 1, &found);
            pos += r; // pos指向找到的delimiter
            if (!found) {
                continue; // 跳过以下所有处理
            }
        } else {
            if ((*pos != '\"') && (*pos != ',') && (*pos != ':') && (*pos != '{') && (*pos != '}')) {
                pos++;
                continue;
            }
        }

        {
            switch (*pos) {
            case '\"':
                consecutive_backslash_num = 0;
                for (int i = 1; ; i++) {
                    if ('\\' == *(pos - i)) {
                        consecutive_backslash_num++;
                    } else {
                        break;
                    }
                }
                if (0 == (consecutive_backslash_num % 2)) {
                    if (!is_in_string) {
                        string_start = pos;
                    } else {
                        string_end = pos;
                        string_len = string_end - string_start - 1; // 不包含双引号的长度
                    }
                    is_in_string = is_in_string ^ 1; // 取反
                }
                break; // 注意这里break的是switch语句
            case ',':
            case '}':
                if (!is_in_string) {
                    if (':' == *prev_delimiter) {
                        if (is_interested) {
                            prefix_space = 0;
                            suffix_space = 0;
                            // 去掉前后空白字符。注意这里可能不是string，所以不能去找双引号。
                            for (int i = 1; ; i++) {
                                if (('\t' == *(prev_delimiter + i)) || ('\n' == *(prev_delimiter + i))
                                        || ('\r' == *(prev_delimiter + i)) || (' ' == *(prev_delimiter + i))) {
                                    prefix_space++;
                                } else {
                                    break;
                                }
                            }
                            for (int i = 1; ; i++) {
                                if (('\t' == *(pos - i)) || ('\n' == *(pos - i))
                                        || ('\r' == *(pos - i)) || (' ' == *(pos - i))) {
                                    suffix_space++;
                                } else {
                                    break;
                                }
                            }
                            enrichees[num_enrichees].orig_value = prev_delimiter + prefix_space + 1;
                            enrichees[num_enrichees].orig_value_len = pos - suffix_space - enrichees[num_enrichees].orig_value;
                            //printf("The value is: %.*s\n", enrichees[num_enrichees].orig_value_len, enrichees[num_enrichees].orig_value);
                            interested->enricher__(&enrichees[num_enrichees]);
                            num_enrichees++;
                            interested = NULL;
                            is_interested = 0;
                        }
                    }
                    prev_delimiter = pos;
                }
                break;
            case ':':
                if (!is_in_string) {
                    if ((',' == *prev_delimiter) || ('{' == *prev_delimiter)) {
                        if (is_pair_interested(string_start, string_len, &interested)) {
                            is_interested = 1;
                        }
                    }
                    prev_delimiter = pos;
                }
                break;
            case '{':
                if (!is_in_string) {
                    prev_delimiter = pos;
                }
                break;
            default:
                ; // 不应该出现这种情况！
            }
        }

        pos++;
    }
    return 0;
}

