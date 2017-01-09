struct enrichee {
    int orig_name_len;
    const char *orig_name;
    int orig_value_len;
    const char *orig_value; // 如果是string的话包括双引号
    int enriched_value_len;
    char *enriched_value;
};
    
typedef int (*enricher)(struct enrichee *enrichee__);
extern int init();
extern int register_enricher(const char *interested_name, enricher enricher__);
extern int extract(const char *buf, const char *buf_end);
