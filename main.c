#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

#define    READ    0      /* The index of the “read” end of the pipe */
#define    WRITE    1      /* The index of the “write” end of the pipe */

//this structre is used to pass the arguments to the thread
struct register_thread {
    FILE *fp;
    struct student *p;
    struct student *last;
};

//this struct stores records of data
struct total_records {
    int totalPositives;
    int totalTests;
    int inClubhouse;
    int inICU;
    int totalRoomQ;
    int totalHosp;
    int avlClubhouse;
    int avlTestkits;
    int avlHospital;
    int avlICU;
};

//this struct stores student information
struct student {
    char name[50];
    int studentid;
    char gender;
    int age;
    long long int aadharNo;
    int isPositive;
    int ctValue;
    int O2Level;
    struct student *next;
};

struct total_records lastData = {0, 0, 0, 0, 0, 0, 0, 0, 0};

struct student *create_node(long long int aadhar, int age, char gender, int s_id, char nm[]) {
    struct student *p = (struct student *) malloc(sizeof(struct student));
    p->age = age;
    p->gender = gender;
    strcpy(p->name, nm);
    p->studentid = s_id;
    p->aadharNo = aadhar;
    p->isPositive = -1;
    p->ctValue = -1;
    p->O2Level = -1;
    p->next = NULL;
    return p;
}

void *register_student(void *arg) {
    struct register_thread *reg = arg;
    FILE *f = reg->fp;
    char nm[50];
    char gender;
    int age, studentid;
    long long int aadhar;
    reg->p = NULL;
    struct student *p;
    while (!feof(f)) {
        fscanf(f, "%lld %d %c %d", &aadhar, &age, &gender, &studentid);

        fgets(nm, 50, f);
        p = create_node(aadhar, age, gender, studentid, nm);
        if (reg->p == NULL) {
            reg->p = p;
            reg->last = p;
        } else {
            reg->last->next = p;
            reg->last = reg->last->next;

        }
    }
    printf("\nRegistration successfully completed!\n");
    pthread_exit(0);
}

void *Testing(void *arg) {
    struct student *phead = arg;
    int ctValue, O2 = 0;

    srand(time(0));

    ctValue = 5 + (rand() % 60);


    if (ctValue > 35) {
        printf("\nAadhar no %lld: Negative.", phead->aadharNo);
        phead->isPositive = 0;
        phead->ctValue = ctValue;
    } else {
        printf("\nAadhar no %lld: Positive.", phead->aadharNo);
        phead->isPositive = 1;
        phead->ctValue = ctValue;

        if (ctValue < 10)
            O2 = 60 + (rand() % 21);
        else if (ctValue > 10 && ctValue <= 25)
            O2 = 81 + (rand() % 10);
        else if (ctValue > 25 && ctValue <= 35)
            O2 = 91 + (rand() % 10);

        phead->O2Level = O2;
    }

    pthread_exit(0);
}

void *make_positive_list(void *arg) {
    struct student *p = arg;
    FILE *fp = fopen("dataFiles/patients_positive.txt", "w");
    if (fp == NULL) {
        printf("\nOperation Failed!!!\n");
        pthread_exit(0);
    }

    while (p->next != NULL) {
        if (p->isPositive == 1) {
            fprintf(fp, "%lld %d %d %d %s", p->aadharNo, p->ctValue, p->O2Level, p->age, p->name);
            lastData.totalPositives++;
        }
        p = p->next;
    }
    fclose(fp);
    pthread_exit(0);
}

void *make_negative_list(void *arg) {
    struct student *p = arg;
    FILE *fp = fopen("dataFiles/patients_negative.txt", "w");
    if (fp == NULL) {
        printf("\nOperation Failed!!!\n");
        pthread_exit(0);
    }
    while (p->next != NULL) {
        if (p->isPositive == 0) {
            fprintf(fp, "%lld %d %s", p->aadharNo, p->age, p->name);
        }
        p = p->next;
    }
    fclose(fp);
    pthread_exit(0);
}

void *emergency(void *arg) {
    int *o2 = arg;
    FILE *clubRooms;
    FILE *icuRooms;
    int ven, icub;
    clubRooms = fopen("dataFiles/Clubhouse.txt", "r");
    icuRooms = fopen("dataFiles/ICU_Beds.txt", "r");
    fscanf(clubRooms, "%d", &ven);
    fscanf(icuRooms, "%d", &icub);
    fclose(clubRooms);
    fclose(icuRooms);
    if (*o2 < 70) {
        if (ven == 0 && icub > 0) {
            printf("\n Clubhouse is full. Please admit to Aashka ICU.\n");
            icub--;
            icuRooms = fopen("dataFiles/ICU_Beds.txt", "w");
            fprintf(icuRooms, "%d", icub);
            fprintf(icuRooms, "\n");
            fclose(icuRooms);
            lastData.inICU++;
        } else if (ven > 0) {
            icub--;
            icuRooms = fopen("dataFiles/ICU_Beds.txt", "w");
            fprintf(icuRooms, "%d", icub);
            fprintf(icuRooms, "\n");
            fclose(icuRooms);
            lastData.inICU++;

            ven--;
            clubRooms = fopen("dataFiles/Clubhouse.txt", "w");
            fprintf(clubRooms, "%d", ven);
            fprintf(clubRooms, "\n");
            fclose(clubRooms);
            lastData.inClubhouse++;

        }
    } else if (*o2 >= 70 && *o2 <= 80) {
        icub--;
        icuRooms = fopen("dataFiles/ICU_Beds.txt", "w");
        fprintf(icuRooms, "%d", icub);
        fclose(icuRooms);
        lastData.inICU++;
    }
    pthread_exit(0);
}

void DisplayTodayData() {
    FILE *kit = fopen("dataFiles/available_kits.txt", "r");
    FILE *bed = fopen("dataFiles/available_beds.txt", "r");
    FILE *vent = fopen("dataFiles/Clubhouse.txt", "r");
    FILE *icu = fopen("dataFiles/ICU_Beds.txt", "r");
    FILE *hosp = fopen("dataFiles/Hospitalize.txt", "r");
    FILE *HQ = fopen("dataFiles/room_quarantine.txt", "r");

    fscanf(bed, "%d", &lastData.avlHospital);
    fclose(bed);

    fscanf(kit, "%d", &lastData.avlTestkits);
    fclose(kit);

    fscanf(vent, "%d", &lastData.avlClubhouse);
    fclose(vent);

    fscanf(icu, "%d", &lastData.avlICU);
    fclose(icu);


    char chr = getc(hosp);
    while (chr != EOF) {
        if (chr == '\n') {
            lastData.totalHosp++;
        }
        chr = getc(hosp);
    }
    fclose(hosp);

    chr = getc(HQ);
    while (chr != EOF) {
        if (chr == '\n') {
            lastData.totalRoomQ++;
        }
        chr = getc(HQ);
    }
    fclose(HQ);

    printf("\n-------Today's--Report-------\n");

    //printing Records
    printf("\nTotal Tests done: %d ", lastData.totalTests);
    printf("\nTotal Positive Cases:  %d\n", lastData.totalPositives);
    printf("\nTotal Room-Quarantined Students: %d", lastData.totalRoomQ);
    printf("\nTotal Available Kits at this point of time in DAIICT: %d\n", lastData.avlTestkits);
    printf("\nTotal Hospitalized Patients : %d", lastData.totalHosp);
    printf("\nTotal General-Ward Beds available in Aashka Hospital: %d\n", lastData.avlHospital);
    printf("\nStudents In ICU : %d , from Which %d patients are in Clubhouse.", lastData.inICU,
           lastData.inClubhouse);
    printf("\nTotal ICU beds Available in Aashka Hospital: %d", lastData.avlICU);
    printf("\nTotal beds available in Clubhouse: %d", lastData.avlClubhouse);
    printf("\n\n-----End--of--Report-----\n");
}

void main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("\n ERR: DATA NOT PROVIDED\n");
        exit(1);
    }

    //registration records
    pthread_t registrar;
    struct register_thread reg;
    int err = -1, ret = -1;

    reg.fp = fopen(argv[1], "r");
    if (reg.fp == NULL) {
        fprintf(stderr, "%s path or file does not exist!\n", argv[1]);
        exit(1);
    }

    err = pthread_create(&registrar, NULL, register_student, (void *) &reg);
    if (err == 0) {}
    else {
        printf("Thread cannot be created: %s\n", strerror(err));
        exit(1);
    }
    err = -1;
    sleep(1);

    err = pthread_join(registrar, (void **) &ret);
    if (err != 0) {
        printf("Joining ERROR : %s\n", strerror(err));
        exit(1);
    }


    //Testing
    pthread_t TestingTeams[5];
    int count = 0, i, *isPositive;
    struct student *phead = reg.p;
    struct student *plast = reg.last;
    int ans[50], ac = 0;

    while (phead != plast) {
        count++;
        err = pthread_create(&TestingTeams[count % 5], NULL, Testing, (void *) phead);
        if (err != 0) {
            printf("Thread cannot be created: %s\n", strerror(err));
            exit(1);
        }
        phead = phead->next;
        sleep(1);
    }

    printf("\n\nNumber of Students registered : %d\n", count);
    lastData.totalTests = count;
    for (i = 0; i < 5; i++) {
        err = pthread_join(TestingTeams[i], (void **) &isPositive);
        if (err != 0) {
            printf("Thread cannot be created: %s\n", strerror(err));
            exit(1);
        }
    }

    //collecting data & segregating
    pthread_t positives, negatives;
    phead = reg.p;

    err = pthread_create(&positives, NULL, make_positive_list, (void *) phead);
    if (err != 0) {
        printf("Thread cannot be created: %s\n", strerror(err));
        exit(1);
    }

    err = pthread_create(&negatives, NULL, make_negative_list, (void *) phead);
    if (err != 0) {
        printf("Thread cannot be created: %s\n", strerror(err));
        exit(1);
    }

    err = pthread_join(positives, (void **) &ret);
    if (err != 0) {
        printf("Thread cannot be created: %s\n", strerror(err));
        exit(1);
    }
    err = pthread_join(negatives, (void **) &ret);
    if (err != 0) {
        printf("Thread cannot be created: %s\n", strerror(err));
        exit(1);
    }

    //treatment
    int child_pid, status;
    int child_write[2];
    pipe(child_write);
    phead = reg.p;
    child_pid = fork();
    if (child_pid == 0) {
        close(child_write[READ]);

        int kit, beds;
        char o2[4];

        FILE *fp_beds;
        FILE *fp_kits;
        FILE *fp_HQ = fopen("dataFiles/room_quarantine.txt", "w");
        FILE *fp_hosp = fopen("dataFiles/Hospitalize.txt", "w");
        while (phead->next != NULL) {
            if (phead->ctValue > 25 && phead->ctValue <= 35) {
                fp_kits = fopen("dataFiles/available_kits.txt", "r");
                fscanf(fp_kits, "%d", &kit);
                fclose(fp_kits);
                if (kit > 0) {
                    kit--;
                    fp_kits = fopen("dataFiles/available_kits.txt", "w");
                    fprintf(fp_kits, "%d", kit);
                    fprintf(fp_kits, "\n");
                    fclose(fp_kits);
                    fprintf(fp_HQ, "%lld %d %d %d %s", phead->aadharNo, phead->ctValue, phead->O2Level, phead->age,
                            phead->name);
                } else {
                    printf("\nNo more kits available! Testing will continue as soon as kits are restocked. \n");
                }
            } else if (phead->ctValue >= 10 && phead->ctValue <= 25) {
                fp_beds = fopen("dataFiles/available_beds.txt", "r");
                fscanf(fp_beds, "%d", &beds);
                fclose(fp_beds);
                if (beds > 0) {
                    beds--;
                    fp_beds = fopen("dataFiles/available_beds.txt", "w");
                    fprintf(fp_beds, "%d", beds);
                    fprintf(fp_beds, "\n");
                    fclose(fp_beds);
                    fprintf(fp_hosp, "%lld %d %d %d %s", phead->aadharNo, phead->ctValue, phead->O2Level, phead->age,
                            phead->name);
                } else {
                    printf("\nBeds full! Trying to contact Other Hospitals...\n");
                }
            } else if (phead->ctValue < 10) {
                fprintf(fp_hosp, "%lld %d %d %d %s", phead->aadharNo, phead->ctValue, phead->O2Level, phead->age,
                        phead->name);
                sprintf(o2, "%d", phead->O2Level);
                write(child_write[WRITE], o2, strlen(o2) + 1);
            }
            phead = phead->next;
        }

        sprintf(o2, "%d", -1);
        write(child_write[WRITE], o2, strlen(o2) + 1);
        close(child_write[WRITE]);
        fclose(fp_hosp);
        fclose(fp_HQ);
        exit(0);

    } else {
        close(child_write[WRITE]);
        int bytesRead, o2 = 99;
        char o2s[4];
        while (1) {
            bytesRead = read(child_write[READ], o2s, strlen(o2s) + 1);

            o2 = atoi(o2s);

            if (o2 == -1) {
                break;
            } else if (o2 >= 60 && o2 <= 80) {
                kill(child_pid, SIGSTOP);

                pthread_t emergence;
                err = pthread_create(&emergence, NULL, emergency, (void *) &o2);
                if (err != 0) {
                    printf("Thread cannot be created: %s\n", strerror(err));
                    exit(1);
                }
                err = pthread_join(emergence, (void **) &ret);
                if (err != 0) {
                    printf("Thread cannot be created: %s\n", strerror(err));
                    exit(1);
                }
                kill(child_pid, SIGCONT);
            }
        }
        close(child_write[READ]);
    }
    child_pid = wait(&status);

    //Display the end of the day Records.
    DisplayTodayData();
}
