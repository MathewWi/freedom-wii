void set_encrypt_iv(u16 index);
void encrypt_buffer(u8 *source, u8 *dest, u32 len);
void aes_set_key(u8 *key);
//void aes_decrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);
//void aes_encrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);
