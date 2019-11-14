#include "http.h" 

int main(void)
{
    http_t *http_packet=malloc(sizeof(http_t));
    http_interpret("test/template/HTTP_GET.txt", http_packet);
    debug_http_obj(http_packet);
    return 0;
}