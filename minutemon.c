/* 
Written By: Todd Nelson, Nelonic Systems, LLC.
Date: 8/27/2012
Version: 1.00
For use with Minuteman Entrust ETR500 UPS. 
Listens to continuous output from UPS and processes useful data.
Can be run as a daemon to continuously monitor UPS or on demand.
*/

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/hiddev.h>
#include <string.h>


#define EV_NUM 5

char* toYesNo(int val) {

	if (val == 1) { return "Yes"; } else { return "No"; }

}

int main (int argc, char **argv) {

  int fd = -1;
  int i;
  struct hiddev_event ev[EV_NUM];
  
  if (argc < 2) {
  printHelp:
	fprintf(stderr, "\n            ******** Minutemon Version: 1.00 ********");
    fprintf(stderr, "\n\nUsage (daemon mode): \n  %s <hid device> <to email> <from email> --daemon\n",argv[0]);
    fprintf(stderr, "  Ex: %s /dev/usb/hiddev0 user@domain.com ups@domain.com --daemon\n", argv[0]);
	fprintf(stderr, "\nUsage (user mode): \n  %s <hid device>\n",argv[0]);
    fprintf(stderr, "  Ex: %s /dev/usb/hiddev0 \n\n", argv[0]);
    exit(1);
  } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
	goto printHelp;
  }
  
  
  if ((fd = open(argv[1], O_RDONLY)) < 0) {
    perror("hiddev open");
    exit(1);
  }
  
  int batChg = -1, needRepl = -1, discharging = -1, charging = -1, belowCapLimit = -1, onMains = -1, intFail = -1, shutImminent = -1, fullyDischarged = -1, fullyCharged = -1;
  char msg[1000], oldMsg[500], chkMsg[500], mailHeader[1000], cmd[100], logMsg[1000];
			 
  while (1) {
      read(fd, ev, sizeof(struct hiddev_event) * EV_NUM);
      for (i = 0; i < EV_NUM; i++) {
            				  
		  if (ev[i].hid == 0x850066) { batChg = ev[i].value; }
		  if (ev[i].hid == 0x85004b) { needRepl = ev[i].value; }
		  if (ev[i].hid == 0x850045) { discharging = ev[i].value; }
		  if (ev[i].hid == 0x850044) { charging = ev[i].value; }
		  if (ev[i].hid == 0x850042) { belowCapLimit = ev[i].value; }
		  if (ev[i].hid == 0x8500d0) { onMains = ev[i].value; }
		  if (ev[i].hid == 0x840062) { intFail = ev[i].value; }
		  if (ev[i].hid == 0x840069) { shutImminent = ev[i].value; }
		  if (ev[i].hid == 0x850047) { fullyDischarged = ev[i].value; }
		  if (ev[i].hid == 0x850046) { fullyCharged = ev[i].value; }
      }
	  
	 if (batChg > -1 && needRepl > -1 && discharging > -1 && charging > -1 && belowCapLimit > -1 && onMains > -1 && intFail > -1 && shutImminent > -1 && fullyDischarged > -1 && fullyCharged > -1) { 
		//All info collected, process...
		
		sprintf(msg, "On Mains: %s\nBattery Charge: %d\nBattery Needs Replacement: %s\nDischarging: %s\nCharging: %s\nBelow Capacity Limit: %s\nInternal Failure: %s\nShutdown Imminent: %s\nFully Discharged: %s\nFully Charged: %s\n", toYesNo(onMains), batChg, toYesNo(needRepl), toYesNo(discharging), toYesNo(charging), toYesNo(belowCapLimit), toYesNo(intFail), toYesNo(shutImminent), toYesNo(fullyDischarged), toYesNo(fullyCharged));
		sprintf(chkMsg, "On Mains: %s\nBattery Needs Replacement: %s\nBelow Capacity Limit: %s\nInternal Failure: %s\nShutdown Imminent: %s\nFully Discharged: %s\n", toYesNo(onMains), toYesNo(needRepl), toYesNo(belowCapLimit), toYesNo(intFail), toYesNo(shutImminent), toYesNo(fullyDischarged));
		
		if (argc < 5) {
			//Not running in daemon mode. Print out results and quit.
			printf(msg);
			exit(0);
		}
		
		if (strcmp(chkMsg, oldMsg) != 0) {
			//UPS Status has changed, send alert.
					
			if (argc >= 4) {
			
			   sprintf(mailHeader,"echo 'To: %s\nFrom: %s\nSubject: Minuteman Power Event\n\n%s ' > email.tmp", argv[2], argv[3], msg);
			   system(mailHeader);
			   sprintf(cmd, "sendmail %s < email.tmp", argv[2]);
			   system(cmd);
			   system("rm email.tmp");
			   sprintf(logMsg, "echo '*******' `date` '*****\n\n%s' >> /var/log/minutemon.log",msg);
			   system(logMsg);
		  
		  } else {
		  
			printf("Not sending email, addresses not properly specified.\n");
			
		  }
		}
	  
	  strcpy(oldMsg, chkMsg);
	  
	  batChg = -1; 
	  needRepl = -1;
	  discharging = -1;
	  charging = -1;
	  belowCapLimit = -1;
	  onMains = -1;
	  intFail = -1;
	  shutImminent = -1;
	  fullyDischarged = -1;
	  fullyCharged = -1;
	}
		
	nanosleep(2000000000);  //pause for 2 seconds, then check UPS again.
 }
  close(fd);
  exit(0);
}

