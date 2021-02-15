/*
 * BREXX/370 - Format
 */

#include "lstring.h"
#include <cmssys.h>
#include <math.h>

typedef struct format_data {
    PLstr to;
    double from;
    int sign;
    char *significant;
    int expnt;
    int form;
    int digits;
    long before;
    long after;
    long expp;
    long expt;
    char buffer[20];
} format_data;

static double round_dec_digits(double value, int digits)
{
    if (value == 0.0)
        return 0.0;

    double factor = pow(10.0, digits);

    /* FOR GCCLIB/370 we need to add an epsilon to make it round the right way */
    /* 15 significant digits can be handled  */
    double epsilon = 1.0 / pow(10.0, 15 - ceil(log10(fabs(value))) - digits);

    return floor(0.5 + epsilon + (value * factor)) / factor;
}

static double round_sig_digits(double value, int digits)
{
    if (value == 0.0)
        return 0.0;

    double factor = pow(10.0, digits - ceil(log10(fabs(value))));

    /* FOR GCCLIB/370 we need to add an epsilon to make it round the right way */
    /* 15 significant digits can be handled  */
    double epsilon = 1.0 / pow(10.0, 15 - digits);

    return floor(0.5 + epsilon + (value * factor)) / factor;
}

static void get_significant_and_expnt(format_data *data) {
    if (data->from == 0.0) data->expnt = 0;
    else data->expnt = (int)floor(log10(data->from) + 1e-14);

    /* Make scientific form with significant as nnnnnnnnnnnnnnn which means n.nnnnnnnnnnnnnn */
    sprintf(data->buffer, "%.*e", data->digits, data->from);
    data->buffer[1] = data->buffer[0];
    data->significant = data->buffer + 1;
    data->significant[data->digits] = 0;

    /* Remove trailing decimal zeros */
    while (data->significant[data->digits - 1] == '0') {
        if (data->digits  == 1) break;
        data->significant[data->digits - 1] = 0;
        data->digits--;
    }
}

static void report_error() {
    Context *context = (Context *) CMSGetPG();
    (context->lstring_Lerror)(ERR_LONGER_RESULT, 0);
}

static void format_simple(format_data *data) {
    int x;
    char *out;

    /*
     * The before and after options describe how many characters are used for
     * the integer and decimal parts of the result, respectively.
     * If you omit either or both of them, the number of characters used for
     * that part is as needed.
     *
     * If after is not the same size as the decimal part of the number, the number
     * is rounded (or extended with zeros) to fit.
     * Specifying 0 causes the number to be rounded to an
     * integer.
     */
    int decimal_needed = data->digits - data->expnt - 1;
    if (decimal_needed < 0) decimal_needed = 0;
    if (data->after != -1 && decimal_needed > data->after) {
        /* Need to round and regenerate significant and exponent */
        data->from = round_dec_digits(data->from,data->after);
        get_significant_and_expnt(data);
        decimal_needed = data->digits - data->expnt - 1;
        if (decimal_needed < 0) decimal_needed = 0;
    }

    /*
     * If before is not large enough to contain the integer part of the number
     * (plus the sign for a negative number), an error results.
     */
    int int_needed = data->expnt + 1;
    if (int_needed < 1) int_needed = 1; /* for '0' */
    if (data->sign < 0) int_needed++; /* for the '-' */
    if (data->before != -1) if (int_needed > data->before) report_error();

    /* Make the variable length big enough */
    Lfx(data->to, int_needed + decimal_needed + 2);
    out = LSTR(*(data->to));

    /*
     * If before is larger than needed for that part, the number i
     * padded on the left with blanks.
     */
    if (data->before != -1) {
        for (x = 0; x < data->before - int_needed; x++) *(out++) = ' ';
    }

    if (data->expnt < 0) {
        /* No int digits */
        if (data->sign < 0) {
            strcpy(out, "-0.");
            out += 3;
        }
        else {
            strcpy(out, "0.");
            out += 2;
        }
        for (x = 0; x < - data->expnt - 1; x++) *(out++) = '0';
        sprintf(out, "%.*s", data->digits, data->significant);
        if (data->after != -1 &&  data->after - decimal_needed > 0) {
            out += strlen(out);
            for (x = 0; x < data->after - decimal_needed; x++) *(out++) = '0';
            *out = 0;
        }
    }
    else if (data->expnt + 1 >= data->digits) {
        /* No decimal digits */
        if (data->sign < 0) *(out++) = '-';
        sprintf(out,"%.*s", data->digits, data->significant);
        out += strlen(out);
        for (x = 0; x < 1 + data->expnt - data->digits; x++) *(out++) = '0';
        if (data->after != -1 &&  data->after - decimal_needed > 0) {
            *(out++) = '.';
            for (x = 0; x < data->after - decimal_needed; x++) *(out++) = '0';
        }
        *out = 0;
    }
    else {
        /* Decimal point in the significant range */
        if (data->sign < 0) *(out++) = '-';

        sprintf(out, "%.*s.%.*s", data->expnt + 1, data->significant,
                data->digits - data->expnt - 1, data->significant + data->expnt + 1);

        if (data->after != -1 &&  data->after - decimal_needed > 0) {
            out += strlen(out);
            for (x = 0; x < data->after - decimal_needed; x++) *(out++) = '0';
            *out = 0;
        }
    }

    /* Fix Length */
    LTYPE(*(data->to)) = LSTRING_TY;
    LLEN(*(data->to)) = strlen(LSTR(*(data->to)));
 }

static void format_sci_or_eng(format_data *data) {

    int int_digits = 1;
    int x;
    char *out;

    if (data->form == ENGINEERING) {
        /* Convert to engineering form */
        x = data->expnt % 3;
        if (x) {
            if (x <= 0) x = 3 + x;
            int_digits += x;
            data->expnt -= x;
        }
    }

    /*
     * The before and after options describe how many characters are used for
     * the integer and decimal parts of the result, respectively.
     * If you omit either or both of them, the number of characters used for
     * that part is as needed.
     *
     * If after is not the same size as the decimal part of the number, the number
     * is rounded (or extended with zeros) to fit.
     * Specifying 0 causes the number to be rounded to an
     * integer.
     */
    int decimal_needed = data->digits - int_digits;
    if (decimal_needed < 0) decimal_needed = 0;

    if (data->after != -1 && decimal_needed > data->after) {
        /* Need to round and regenerate significant and exponent, etc. */
        data->from = round_dec_digits(data->from,data->after - data->expnt);
        get_significant_and_expnt(data);
        if (data->form == ENGINEERING) {
            x = data->expnt % 3;
            if (x) {
                if (x <= 0) x = 3 + x;
                int_digits += x;
                data->expnt -= x;
            }
        }
        decimal_needed = data->digits - int_digits;
        if (decimal_needed < 0) decimal_needed = 0;
    }

    /*
     * If before is not large enough to contain the integer part of the number
     * (plus the sign for a negative number), an error results.
     */
    int int_needed = int_digits;
    if (int_needed < 1) int_needed = 1; /* for '0' */
    if (data->sign < 0) int_needed++; /* for the '-' */
    if (data->before != -1) if (int_needed > data->before) report_error();

    /* Make the variable length big enough */
    Lfx(data->to, int_needed + decimal_needed + 10); /* Rough and Ready */
    out = LSTR(*(data->to));

    /*
     * If before is larger than needed for that part, the number i
     * padded on the left with blanks.
     */
    if (data->before != -1) {
        for (x = 0; x < data->before - int_needed; x++) *(out++) = ' ';
    }

    if (data->digits - int_digits) {
        sprintf(out, "%s%.*s.%.*s", data->sign < 0 ? "-" : "", int_digits,
                data->significant,
                data->digits - int_digits, data->significant + int_digits);
        out += strlen(out);
        if (data->after != -1 &&  data->after - decimal_needed > 0) {
            for (x = 0; x < data->after - decimal_needed; x++) *(out++) = '0';
        }
    }
    else {
        sprintf(out, "%s%*s", data->sign < 0 ? "-" : "", int_digits,
                data->significant);
        out += strlen(out);
        if (data->after != -1 &&  data->after - decimal_needed > 0) {
            *(out++) = '.';
            for (x = 0; x < data->after - decimal_needed; x++) *(out++) = '0';
        }
    }

    /* If the exponent would be 0 when a nonzero expp is specified,
       then expp+2 blanks are supplied for the exponent part of the result. */
    if (data->expp > 0 && data->expnt == 0) {
        for (x = 0; x < data->expp + 2; x++) *(out++) = ' ';
        *out = 0;
    }
    else sprintf(out,"E%+.*d", (int)data->expp, data->expnt);

    /* Fix Length */
    LTYPE(*(data->to)) = LSTRING_TY;
    LLEN(*(data->to)) = strlen(LSTR(*(data->to)));
}

/* ---------------- Lformat ------------------ */
void __CDECL
Lformat(const PLstr to, const PLstr from,
        long before, long after, long expp, long expt) {

    format_data data;
    format_data temp;
    double l;
    int int_needed;
    int decimal_needed;
    int log_needed;
    int use_simple = 1;
    double from_d;

    Context *context = (Context *) CMSGetPG();

    data.sign = 1;
    data.to = to;
    data.before = before;
    data.after = after;
    data.expp = expp;
    data.expt = expt;

    from_d = Lrdreal(from);

    data.digits = context->lstring_lNumericDigits;
    if (data.digits > 15) data.digits = 15;
    else if (data.digits < 1) data.digits = 1;

    /*
     * The number is first rounded according to standard Rexx rules, as though
     * the operation number+0 had been carried out.
     */
    if (data.digits < 15) from_d = round_sig_digits(from_d, data.digits);
    if (from_d < 0) {
        data.sign = -1;
        from_d = from_d * -1.0;
    }
    data.from = from_d;

    get_significant_and_expnt(&data);


    /* Calculate decimal and int needed ASSUMING simple format */
    decimal_needed = data.digits - data.expnt - 1;
    if (decimal_needed < 0) decimal_needed = 0;
    int_needed = data.expnt + 1;
    if (int_needed < 1) int_needed = 1; /* for '0' */
    if (data.sign < 0) int_needed++; /* for the '-' */

    if (data.after != -1 && decimal_needed > data.after) {
        /* Need to round and regenerate significant and exponent */
        memcpy(&temp, &data, sizeof(data));
        data.from = round_dec_digits(data.from,data.after);
        get_significant_and_expnt(&data);
        decimal_needed = data.digits - data.expnt - 1;
        if (decimal_needed < 0) decimal_needed = 0;

        int_needed = data.expnt + 1;
        if (int_needed < 1) int_needed = 1; /* for '0' */
        if (data.sign < 0) int_needed++; /* for the '-' */

        /* OK Put the data back as this may not be right
         * if we need scientific form */
        memcpy(&data, &temp, sizeof(data));
    }

    if (data.expnt > 0) log_needed = (int)log10(data.expnt) + 2;
    else if (data.expnt < 0) log_needed = (int)log10(-data.expnt) + 2;
    else log_needed = 0;

    /*
     * expt specifies when the exponential expression is used. The default is
     * the current setting of NUMERIC DIGITS.
     * -1 means expt not specified on command (use default)
     */
    if (data.expt == -1) data.expt = context->lstring_lNumericDigits;

    /*
     * expp sets the number of places for the exponent part; the default
     * is to use as many as needed (which can be zero).
     * -1 means expt not specified on command (use default)
     */
    if (data.expp) { /* If expp is 0, the number is not in exponential notation. */
        /*
         * If expt is 0, the exponential notation is always used
         * we ignore "unless the exponent would be 0".
        */
        if (data.expt == 0) use_simple = 0;

            /*
             * If the number of places needed for the integer or decimal part exceeds
             * expt or twice expt, respectively, the exponential notation is used.
             */
        else if (data.expt && (int_needed > data.expt || decimal_needed > data.expt * 2))
            use_simple = 0;

            /* Weird "Digits after point rule for exponentiation" in spec code */
            /* "More than 5 zeros to the right of the point" */
            /* Seems to mean if exponent < -6 then use exponent form */
        else if (data.expnt < -6 ) use_simple = 0;
    }

    /*
     * If the exponent would be 0 and expp is not specified, the number
     * is not an exponential expression.
     */
    if (!data.expnt && data.expp == -1) use_simple = 1;

    if (use_simple) format_simple(&data);

    else {
        data.form = (context->rexx_proc)[(context->rexx_rx_proc)].form;
        format_sci_or_eng(&data);
    }
} /* Lformat */
