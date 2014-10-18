#include "mainvpd_lib.h"

int readMain_I2C_eeprom (char * vpd_str , int topOrBottom , int startAddress , int length);
#ifdef SP2x
	  uint8_t I2C_DC2DC_SWITCH = I2C_DC2DC_SWITCH_GROUP0;
#else
	  uint8_t I2C_DC2DC_SWITCH = I2C_DC2DC_SWITCH_GROUP1;
#endif

		const char *  FET_CODE_2_STR [] = {
				"72A", //0
				"72B", //1
				"72A3P", //2
				"72B3P", //3
				"72A50A", //4
				"72B50A", //5
				"N/A", //6
				"N/A", //7
				"78A50A", //8
				"78B50A", //9 integer (unsigned) [ 0=72A[4P40A] , 1=72B[4P40A],2=72A3P[50A],3=72B3P[50A],4=72A4P50A,5=72B4P50A,6=N/A,7=N/A,8=78A[4P50A],9=78B[4P50A],10=78A3P[50A],11=78B3P[50A] 100=N/A
				"78A3P", //10
				"78B3P", //11
				"N/A", //12
				"N/A", //13
				"N/A", //14
				"N/A" //15
		};

void resetI2CSwitches(){
	int err;
	i2c_write(I2C_DC2DC_SWITCH, PRIMARY_I2C_SWITCH_DEAULT , &err);
	usleep(2000);

	i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT , &err);
	usleep(2000);
}

inline int fix_max_cap(int val, int max){
	if (val < 0)
		return 0;
	else if (val > max)
		return max;
	else
		return val;
}


int usage(char * app ,int exCode ,const char * errMsg)
{
    if (NULL != errMsg )
    {
        fprintf (stderr,"==================\n%s\n\n==================\n",errMsg);
    }
    
    printf ("Usage: %s [-top|-bottom][[-q] [-a] [-p] [-s] [-r] [-f]] [-v vpd] [-vf fet]\n\n" , app);

    printf ("       -top : top main board\n");
    printf ("       -bottom : bottom main board (default)\n");
    printf ("        default mode is read\n");
    printf ("       -q : quiet mode, values only w/o headers\n"); 
    printf ("       -a : print all VPD params as one string\n"); 
    printf ("       -p : print part number\n"); 
    printf ("       -r : print revision number\n"); 
    printf ("       -s : print serial number\n");
    printf ("		-f : print FET type and code\n");
    printf ("       -v : vpd value to write\n");
    printf ("       -vf : fet value to write (integer)\n");
    if (0 == exCode) // exCode ==0 means - just print usage and get back to app for business. other value - exit with it.
    {
        return 0;
    }
    else 
    {
        exit(exCode);
    }
}

int usage(char * app ,int exCode)
{
     return usage(app ,exCode ,NULL) ;
}

int setI2CSwitches(int tob){

	int err = 0;
	int I2C_MY_MAIN_BOARD_PIN = (tob == TOP_BOARD)?PRIMARY_I2C_SWITCH_BOARD0_MAIN_PIN : PRIMARY_I2C_SWITCH_BOARD1_MAIN_PIN;
	i2c_write(PRIMARY_I2C_SWITCH, I2C_MY_MAIN_BOARD_PIN | PRIMARY_I2C_SWITCH_DEAULT , &err);

	if (err==0){
		usleep(2000);
		i2c_write(I2C_DC2DC_SWITCH,  PRIMARY_I2C_SWITCH_DEAULT , &err);
	}
	else{
		fprintf(stderr,"failed calling i2c set 0x70 %x\n", I2C_MY_MAIN_BOARD_PIN | PRIMARY_I2C_SWITCH_DEAULT);
		return err;
	}

	if (err==0){
		usleep(2000);
		i2c_write(I2C_DC2DC_SWITCH,  MAIN_BOARD_I2C_SWITCH_EEPROM_PIN , &err);
		usleep(2000);
	}
	else{
		fprintf(stderr,"failed calling i2c set 0x71 %x\n", PRIMARY_I2C_SWITCH_DEAULT);
		return err;
	}

	return err;
}

int set_fet(int topOrBottom , int fet_type){
	int err=0;
	int rc = 0;
	int vpdrev = 0;

	if (fet_type < 0 || fet_type > 15){
		fprintf(stderr , "FET TYPE %d is illegal (0-15)\n" , fet_type);
		return 5;
	}
	pthread_mutex_lock(&i2c_mutex);

	err = setI2CSwitches(topOrBottom);

	vpdrev =  (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , &err);

	if (vpdrev == 0xFF){
		i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , MAIN_VPD_REV , &err);
		usleep(2000);
		for (int a = 1 ; a < MAIN_BOARD_VPD_EEPROM_SIZE ; a++){
			i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, a , 0 , &err);
			usleep(2000);
		}
	}
	else if ( vpdrev < MAIN_VPD_REV) {
		i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , MAIN_VPD_REV , &err);
		usleep(2000);
	}

	i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, MAIN_BOARD_VPD_FET_FLAG_ADDR_START , 1 , &err);
	usleep(2000);
	i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, MAIN_BOARD_VPD_FET_CODE_ADDR_START , fet_type , &err);
	usleep(2000);

	const char * fetstr = FET_CODE_2_STR[fet_type];
	unsigned b;
	for (b = 0 ; b < strlen(fetstr); b++)
	{
		i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, b+MAIN_BOARD_VPD_FET_STR_ADDR_START  , fetstr[b] , &err);
		usleep(2000);
		if(err != 0)
			break;
	}
	i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, b+MAIN_BOARD_VPD_FET_STR_ADDR_START  , '\0' , &err);

	resetI2CSwitches();
	pthread_mutex_unlock(&i2c_mutex);
	return rc;
}

int get_fet(int topOrBottom ){
	int err=0;
	int fet_type  = 100;
	int fetflag = 0;

	pthread_mutex_lock(&i2c_mutex);
	err = setI2CSwitches(topOrBottom);

	int vpdrev =  (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , &err);

	if (vpdrev == 0xFF) {
	//	fprintf(stderr,"VPD not written at all\n");
		goto get_out;
	}
	else if (vpdrev < FET_SUPPORT_VPD_REV){
//		fprintf(stderr,"VPD from revision %d (need at least %d to include FET information)\n", vpdrev , FET_SUPPORT_VPD_REV);
		goto get_out;
	}

	fetflag =  (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, MAIN_BOARD_VPD_FET_FLAG_ADDR_START , &err);

	if (fetflag != 1)
	{
	//	fprintf(stderr,"VPD does not include FET information\n");
		goto get_out;
	}

	fet_type = (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, MAIN_BOARD_VPD_FET_CODE_ADDR_START, &err);

	get_out:
	resetI2CSwitches();
	pthread_mutex_unlock(&i2c_mutex);
	return fet_type;
}

int get_fet_str(int topOrBottom , char * fet){
	int err=0;
	int rc=0;
	int fetflag = 0;

	pthread_mutex_lock(&i2c_mutex);

	err = setI2CSwitches(topOrBottom);

	int vpdrev =  (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , &err);

	if (vpdrev == 0xFF) {
		fprintf(stderr,"VPD not written at all\n");
		rc  = 1;
		goto get_out;
	}
	else if (vpdrev < FET_SUPPORT_VPD_REV){
		fprintf(stderr,"VPD from revision %d (need at least %d to include FET information)\n", vpdrev , FET_SUPPORT_VPD_REV);
		rc = 2;
		goto get_out;
	}

	fetflag =  (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, MAIN_BOARD_VPD_FET_FLAG_ADDR_START , &err);

	if (fetflag != 1)
	{
		fprintf(stderr,"VPD does not include FET information\n");
		rc = 3;
		goto get_out;
	}

	get_out:
	resetI2CSwitches();
	pthread_mutex_unlock(&i2c_mutex);

	if (rc == 0){
		readMain_I2C_eeprom(fet , topOrBottom , MAIN_BOARD_VPD_FET_STR_ADDR_START , MAIN_BOARD_VPD_FET_STR_ADDR_LENGTH);
	}
	return  rc;
}


int mainboard_set_vpd(  int topOrBottom, const char * vpd){

	int rc = 0;
	int err=0;

	pthread_mutex_lock(&i2c_mutex);
	err = setI2CSwitches(topOrBottom);

	int vpdrev =  (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , &err);
	if (vpdrev == 0xFF){
			i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , MAIN_VPD_REV , &err);
			usleep(2000);
			for (int a = 1 ; a < MAIN_BOARD_VPD_EEPROM_SIZE ; a++){
				i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, a , 0 , &err);
				usleep(2000);
			}
	}
	else if ( vpdrev < MAIN_VPD_REV) {
			i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, 0 , MAIN_VPD_REV , &err);
			usleep(2000);
	}

	for (int b = 0 ; b < (MAIN_BOARD_VPD_FET_STR_ADDR_START-MAIN_BOARD_VPD_SERIAL_ADDR_START); b++)
	{
		i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, b+MAIN_BOARD_VPD_SERIAL_ADDR_START  , vpd[b] , &err);
		usleep(2000);
		if(err != 0)
			break;
	}

	if (err!=0)
	{
		rc = err;
	}

	resetI2CSwitches();

	pthread_mutex_unlock(&i2c_mutex);

	return rc;

}

int readMain_I2C_eeprom (char * vpd_str , int topOrBottom , int startAddress , int length){
	int rc = 0;
	int err=0;

	pthread_mutex_lock(&i2c_mutex);

	err = setI2CSwitches(topOrBottom);

	if (err==0){
		for (int i = 0; i < length; i++) {
			vpd_str[i] = (unsigned char)i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR, (unsigned char)(startAddress+i), &err);
			if (err!= 0)
				break;
		}
	}

	if (err!=0)
	{
		rc = err;
	}

	resetI2CSwitches();

	pthread_mutex_unlock(&i2c_mutex);

	return rc;
}

int mainboard_get_vpd(int topOrBottom ,  mainboard_vpd_info_t *pVpd) {
	int rc = 0;
	int err = 0;

	char vpd_str[32];

	if (pVpd == NULL ) {
		fprintf(stderr,"call mainboard_get_vpd performed without allocating sturcture first\n");
		return 1;
	}

	err = readMain_I2C_eeprom(vpd_str , topOrBottom , 0 , 32);

	strncpy(pVpd->pnr, vpd_str + MAIN_BOARD_VPD_PNR_ADDR_START, MAIN_BOARD_VPD_PNR_ADDR_LENGTH );
	pVpd->pnr[MAIN_BOARD_VPD_PNR_ADDR_LENGTH] = '\0';

	strncpy(pVpd->revision, vpd_str + MAIN_BOARD_VPD_PNRREV_ADDR_START, MAIN_BOARD_VPD_PNRREV_ADDR_LENGTH );
	pVpd->revision[MAIN_BOARD_VPD_PNRREV_ADDR_LENGTH] = '\0';

	strncpy(pVpd->serial, vpd_str + MAIN_BOARD_VPD_SERIAL_ADDR_START, MAIN_BOARD_VPD_SERIAL_ADDR_LENGTH );
	pVpd->serial[MAIN_BOARD_VPD_SERIAL_ADDR_LENGTH] = '\0';

	if (err) {
		fprintf(stderr, RED            "Failed reading %s Main Board VPD (err %d)\n" RESET,(topOrBottom==TOP_BOARD)?"TOP":"BOTTOM", err);
		rc =  err;
	}
	return rc;
}

