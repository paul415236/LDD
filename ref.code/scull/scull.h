#ifndef SCULL_QUANTUM
#define SCULL_QUANTUM 4000
#endif

#ifndef SCULL_QSET
#define SCULL_QSET 1000
#endif

#ifndef SCULL_MAJOR
#define SCULL_MAJOR 0
#endif

#ifndef SCULL_MINOR
#define SCULL_MINOR 0
#endif

/* Scull quantum set */
struct scull_qset {
    void **data;
    struct scull_qset *next;
};

/* Scull device */
struct scull_dev {
    struct cdev cdev; /* Char device structure */
    int quantum; /* quantum size */
    int qset; /* array size */
    unsigned long size; /* amount of data stored */
    struct scull_qset *data; /* First qset in list */
};
