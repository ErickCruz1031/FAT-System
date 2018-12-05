#include <stdio.h>

typedef struct{
    unsigned int DYear;
    unsigned char DMonth;
    unsigned char DDay;
    unsigned char DHour;
    unsigned char DMinute;
    unsigned char DSecond;
    unsigned char DHundredth;
} SVMDateTime, *SVMDateTimeRef;

void ParseDateTime(SVMDateTimeRef datetime_ref, unsigned short date, unsigned short time, unsigned int hundredth);
void ParseDateTime(SVMDateTimeRef datetime_ref, unsigned short date, unsigned short time, unsigned int hundredth);

void ParseDateTime(SVMDateTimeRef datetime_ref, unsigned short date, unsigned short time, unsigned int hundredth){
    // parse the date into month, day, year
    unsigned int year;
    unsigned char month;
    unsigned char day;
    year = (unsigned int) (date & 0b1111111);
    month = (unsigned char) ((date >> 7) & 0b1111);
    day = (unsigned char) ((date >> 11) & 0b11111);

    // parse the time into hour, minute, second, hundredth
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
    unsigned char hundred;

    hour = (unsigned char) (time & 0b11111);
    minute = (unsigned char) ((time >> 6) & 0b111111);
    second = (unsigned char) ((time >> 11) & 0b11111);

    datetime_ref->DYear = year;
    datetime_ref->DMonth = month;
    datetime_ref->DDay = day;
    datetime_ref->DHour = hour;
    datetime_ref->DMinute = minute;
    datetime_ref->DSecond = second;
    datetime_ref->DHundredth = (unsigned char) hundred;

}	

void ParseDateTimeTest(){
    unsigned short date = (unsigned short) 0b0000100100001101;
    unsigned short time = (unsigned short) 0b0000100011100001;

    SVMDateTime datetime;
    ParseDateTime(&datetime, date, time, 0);
    printf("%04d/%02d/%02d\n", datetime.DYear, datetime.DMonth, datetime.DDay);

    return;
}

int main(){
    ParseDateTimeTest();
	return 0;
}
