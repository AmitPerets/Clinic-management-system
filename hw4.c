
#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdlib.h>

//Patiens number
#define N 10

//Node for the patients queues
typedef struct Node
{
    int num;
    struct Node* next;
}*PNode;


//The semaphores
sem_t mutex; //General Lock 
sem_t Clinic; //Semphone for petients in clinic
sem_t Couch; //Semphone for petients on the couch
sem_t Treatment; //Semphone for petients in treament
sem_t WorkingDental; //Semphone for dentals who
sem_t DentalFree; //Semphone that checks if the dental can take payment
sem_t Payment; ////Semphone for payment
sem_t PaymentTransfer; //Semphone for transfering the money

int PatientsInClinic = 0; //The number of patients in the clinic
PNode StandingHead = NULL, CouchHead = NULL; //Linked Lists for the queues

//Function for errors
void Error(char* msg)
{
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

//Function that returns the first patient on the queue
PNode getPatient(PNode head, int* curStand)
{
    if (head == NULL)
        return head; //Returning that the queue is empty
    PNode patient = head;
    *curStand = head->num; //Updating the number of the first patient 
    head = head->next; //Advancing the queue
    free(patient); //Freeing the memory for the first patient 
    return head;
}


//Function for adding patient to a queue
PNode addPatient(PNode head, int num) {
    PNode patient = (PNode)malloc(sizeof(struct Node)); //Allocating memory for the patient
    if (!patient)
        Error("Allocation memory failled");
    patient->num = num;
    patient->next = NULL;
    if (head == NULL) //If there's no one on the queue putting the patient first
        head = patient;
    else { //If there are patients on the queue, putting this one at the end of the queue
        PNode temp = head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = patient;
    }
    return head;
}




//Function for Patient's actions
void* Patient(void* num)
{
    int* patient = (int*)num;

    while (1)
    {
        sem_wait(&mutex); //Locking so only one patient can be checked
        if (PatientsInClinic < N) {
            sem_post(&mutex); //Letting other patients to get checked
            sem_wait(&mutex); //Locking so we can add one node for patient
            StandingHead = addPatient(StandingHead, *patient); //Adding one to the standing queue
            sem_post(&mutex);
            sem_wait(&mutex);
            PatientsInClinic++; // Updating patients num 
            sem_post(&mutex); //Releasing the lock
            printf("\nI'm Patient #%d, I got into clinic", *patient);
            sleep(1);
            sem_wait(&Couch);    //Waiting for the patient to sit on the couch
            sem_wait(&mutex); //Locking so we can remove only one patient from standing to sitting on the couch
            StandingHead = getPatient(StandingHead, patient);  //Getting the first patient who is standing
            sem_post(&mutex); //Releasing the lock
            printf("\nI'm Patient #%d, I'm sitting on the sofa", *patient);
            CouchHead = addPatient(CouchHead, *patient); //Adding one to the sitting queue 
            sleep(1);
            sem_wait(&Treatment); //Waiting for treatment
            sem_wait(&mutex); //Locking so we can remove only one patient from the couch
            CouchHead = getPatient(CouchHead, patient); //Getting the first sitting patient to get off the couch
            sem_post(&mutex); //Releasing the lock
            printf("\nI'm Patient #%d, I'm getting treatment", *patient);
            sleep(1);
            sem_post(&Couch); //Giving the couch it's extra place    
            sem_post(&WorkingDental); //Getting a dental ready to treat a new patient
            sem_wait(&DentalFree); //Waiting for a dental to be able to take money
            printf("\nI'm Patient #%d, I'm paying now", *patient);
            sem_post(&Payment); //Paying to the dental 
            sem_wait(&PaymentTransfer); //Waiting for the dental to accept the payment
            sleep(1);
        }
        else
        {
            sem_post(&mutex); //Releasing the lock
            printf("\nI'm Patient #%d, I'm out of clinic", *patient);
            sleep(1);
            sem_wait(&Clinic); //Waiting that a patient will go out of the clinic
        }
    }
}


//Function for Dental's actions
void* Dental(void* num)
{
    int* DentalNum = (int*)num;
    while (1) {
        sem_wait(&WorkingDental); ///Waiting for a patient to be ready for treatment
        printf("\nI'm Dental Hygienist #%d, I'm working now", *DentalNum);
        sem_post(&Treatment); //A patient finished treatment
        sem_post(&DentalFree); //The patient can pay (the dental is free to take money)
        sleep(1);
        sem_wait(&Payment); //Waiting for the patient's payment
        printf("\nI'm Dental Hygienist #%d, I'm getting a payment", *DentalNum);
        sem_post(&PaymentTransfer); // Accepting the patient's payment 
        sleep(1);
        sem_wait(&mutex); //Locking so we can remove only one patient 
        PatientsInClinic--;
        sem_post(&mutex); //Releasing the lock
        sem_post(&Clinic); //The clinic is free for one more patient
    }
}


void main()
{
    pthread_t Dentals[3], Patients[N + 2]; //An array of threads for patients and array for dentals
    int PatientsIndexes[N + 2]; //An array for Patients thread's indexes
    int DentalsIndexes[3];
    int i;
    //Init semaphores
    sem_init(&Clinic, 0, 0);
    sem_init(&Couch, 0, 4);
    sem_init(&Treatment, 0, 3);
    sem_init(&PaymentTransfer, 0, 1);
    sem_init(&WorkingDental, 0, 0);
    sem_init(&Payment, 0, 0);
    sem_init(&mutex, 0, 1);
    sem_init(&DentalFree, 0, 0);
    //Creating the threads for each patient
    for (i = 0; i < N + 2; i++) {
        PatientsIndexes[i] = i;
        pthread_create(&Patients[i], NULL, Patient, (void*)(PatientsIndexes + i));
    }
    //Creating threads for each dental
    for (i = 0; i < 3; i++) {
        DentalsIndexes[i] = i;
        pthread_create(&Dentals[i], NULL, Dental, (void*)(DentalsIndexes + i));
    }
    //finishing the threads
    for (i = 0; i < N + 2; i++) {
        pthread_join(Patients[i], NULL);
    }
    //finishing the threads
    for (i = 0; i < 3; i++)
        pthread_join(Dentals[i], NULL);
}

