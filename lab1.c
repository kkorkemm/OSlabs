#include <stdio.h>
#include <pthread.h>
#include <unistd.h> 

pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int ready = 0;

// Поставщик
void* provide(void* args) {
    for(int i = 1; i < 10; i++) {
        // Задержка в 1 секунду
        sleep(1);

        // Блокируем мьютекс для безопасного доступа к разделяемому ресурсу
        pthread_mutex_lock(&lock);
        
        if (ready == 1)
        {
            pthread_mutex_unlock(&lock);
            continue;
        }

        ready = 1;
        printf("provided\n");
        pthread_cond_signal(&cond1);
        pthread_mutex_unlock(&lock);
    }
}

// Потребитель
void* consume(void* args) {
    while (1) {
        pthread_mutex_lock(&lock);

        // Ожидаем события с минимальным потреблением процессорного времени
        while (!ready) {
            pthread_cond_wait(&cond1, &lock);
            printf("awoke\n");
        }

        // Событие обработано
        printf("consumed\n");
        ready = 0;

        pthread_mutex_unlock(&lock);
    }
}

int main() {
    pthread_t producer_thread, consumer_thread;

    pthread_create(&producer_thread, NULL, provide, NULL);
    pthread_create(&consumer_thread, NULL, consume, NULL);

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond1);

    return 0;
}