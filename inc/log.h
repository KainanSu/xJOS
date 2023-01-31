#ifndef JOS_INC_LOG_H
#define JOS_INC_LOG_H

#define LOG_TRACE   1
#define LOG_DEBUG   2
#define LOG_INFO    3
#define LOG_WARNING 4
#define LOG_ERROR   5
#define LOG_NONE    6

#define LOG_LEVEL LOG_DEBUG
#define DELAY_LEVEL 0

extern int cprintf(const char *fmt, ...);
extern int vcprintf(const char *fmt, va_list ap);

static void log_out(char* tag, char *fmt, ...){

#ifdef JOS_KERNEL
    cprintf("KERN_%s: ", tag);
#endif
#ifdef JOS_USER
    extern const volatile struct Env *thisenv;
    cprintf("%08x_%s: ", thisenv->env_id ,tag);
#endif

    va_list ap;
	va_start(ap, fmt);
	vcprintf(fmt, ap);
	va_end(ap);

    for(int k = 0; k < DELAY_LEVEL; k++)
        for(int i = 0; i < 1000; i++)
            for(int j = 0; j < 5000; j++)
                ;

}

#if (LOG_TRACE >= LOG_LEVEL)
#define log_trace(fmt, args...) log_out("TRACE", fmt, ## args)
#else
#define log_trace(fmt, args...)  
#endif

#if (LOG_DEBUG >= LOG_LEVEL)
#define log_debug(fmt, args...) log_out("DEBUG", fmt, ## args)
#else
#define log_debug(fmt, args...)  
#endif

#if (LOG_INFO >= LOG_LEVEL)
#define log_info(fmt, args...) log_out("INFO", fmt, ## args)
#else
#define log_info(fmt, args...)  
#endif

#if (LOG_WARNING >= LOG_LEVEL)
#define log_warn(fmt, args...) log_out("WARN", fmt, ## args)
#else
#define log_warn(fmt, args...)  
#endif

#if (LOG_ERROR >= LOG_LEVEL)
#define log_err(fmt, args...) log_out("ERR", fmt, ## args)
#else
#define log_err(fmt, args...)  
#endif

#endif
