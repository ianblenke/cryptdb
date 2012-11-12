#include "aes_cbc.hh"
#include <util/util.hh>

using namespace std;


aes_cbc::aes_cbc(std::string passwd, bool det) {
    enc_key = get_AES_enc_key(passwd);
    dec_key = get_AES_dec_key(passwd);
    is_det = det;
    det_salt = "1234567890123456";
}

string
aes_cbc::encrypt(string pt) {
  
    if (is_det) {
	return encrypt_AES_CBC(pt, enc_key, det_salt);
    } else {
	string rnd_salt = randomBytes(AES_BLOCK_BYTES);
	string ct = encrypt_AES_CBC(pt, enc_key, rnd_salt);
	return rnd_salt + ct;
    }
      
}

string
aes_cbc::decrypt(string ct) {
    if (is_det) {
	return decrypt_AES_CBC(ct, dec_key, det_salt);
    } else {
	string rnd_salt = ct.substr(0, AES_BLOCK_BYTES);
	ct = ct.substr(AES_BLOCK_BYTES, ct.size() - AES_BLOCK_BYTES);
	return decrypt_AES_CBC(ct, dec_key, rnd_salt);
    }
}

