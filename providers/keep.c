/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include <curl/curl.h>
#include <glib.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../common.h"
#include "../ipc.h"
#include "../json.h"
#include "../util.h"

#define GOOGLE_API_KEY                                        \
  "AAAAgMom/1a/v0lblO2Ubrt60J2gcuXSljGFQXgcyZWveWLEwo6prwgi3" \
  "iJIZdodyhKZQrNWp5nKJ3srRXcUW+F1BD3baEVGcmEgqaLZUNBjm057pK" \
  "RI16kB0YppeGx5qIQ5QjKzsR8ETQbKLNWgRY0QRNVz34kMJR3P/LgHax/" \
  "6rmf5AAAAAwEAAQ=="

#define CLIENT_SIG "38918a453d07199354f8b19af05ec6562ced5788"

#define MASTER_TOKEN_REQUEST                               \
  "accountType=HOSTED_OR_GOOGLE&"                          \
  "Email=%s&"                                              \
  "has_permission=1&add_account=1&"                        \
  "EncryptedPasswd=%s&"                                    \
  "service=ac2dm&source=android&androidId=31021392710718&" \
  "device_country=us&operatorCountry=us&"                  \
  "lang=en&sdk_version=17&"                                \
  "client_sig=" CLIENT_SIG "&callerSig=" CLIENT_SIG        \
  "&"                                                      \
  "droidguard_results=dummy123"

#define FINAL_TOKEN_REQUEST                                                   \
  "accountType=HOSTED_OR_GOOGLE&"                                             \
  "Email=janis19011943%%40gmail.com&"                                         \
  "has_permission=1&"                                                         \
  "EncryptedPasswd=%s&"                                                       \
  "service=oauth2%%3Ahttps%%3A%%2F%%2Fwww.googleapis.com%%2Fauth%%2Fmemento+" \
  "https%%3A%%2F%%2Fwww.googleapis.com%%2Fauth%%2Freminders&"                 \
  "source=android&androidId=31021392710718&"                                  \
  "app=com.google.android.keep&"                                              \
  "client_sig=" CLIENT_SIG                                                    \
  "&"                                                                         \
  "device_country=us&operatorCountry=us&"                                     \
  "lang=en&sdk_version=17"

#define ALL_NOTES_REQUEST                                                      \
  "{\"nodes\": [], \"clientTimestamp\": \"%s\", "                              \
  "\"requestHeader\": {\"clientSessionId\": "                                  \
  "\"s--%lld--%lld\", \"clientPlatform\": \"ANDROID\", "                       \
  "\"clientVersion\": {\"major\": \"9\", \"minor\": \"9\", \"build\": \"9\", " \
  "\"revision\": \"9\"}, \"capabilities\": [{\"type\": \"NC\"}, {\"type\": "   \
  "\"PI\"}, {\"type\": \"LB\"}, {\"type\": \"AN\"}, {\"type\": \"SH\"}, "      \
  "{\"type\": \"DR\"}, {\"type\": \"TR\"}, {\"type\": \"IN\"}, {\"type\": "    \
  "\"SNB\"}, {\"type\": \"MI\"}, {\"type\": \"CO\"}]}}"

struct curl_slist* headers;
struct curl_slist* final_headers;
extern int enable_debug;

typedef enum {
  CLIENT_AUTH,
  CLIENT_KEEP,
} ClientType;

static int32_t int32_from_bytearr(unsigned char* b, int offset) {
  return (0xff & b[offset] << 24) + (0xff & b[(offset + 1)] << 16) +
         (0xff & b[(offset + 2)] << 8) + (0xff & b[(offset + 3)]);
}

static unsigned char* bytearr_sub(unsigned char* src, int lindex, int rindex) {
  unsigned char* sub = malloc_or_die(rindex - lindex);

  size_t idx = 0;
  for (size_t i = lindex; i < rindex; i++) {
    sub[idx] = src[i];
    idx++;
  }

  return sub;
}

static BIGNUM* BN_from_bytearr(unsigned char* src, int sz) {
  return BN_bin2bn(src, sz, NULL);
}

/* solely for debugging */
static void print_bytes(unsigned char* buf, int len) {
  for (int i = 0; i < len; i++) printf("Byte %d: 0x%0.02x\n", i, buf[i]);
}

static RSA* RSA_from_mod_exponent(BIGNUM* mod, BIGNUM* exponent) {
  RSA* rsa = RSA_new();
  RSA_set0_key(rsa, mod, exponent, NULL);

  return rsa;
}

#define B64_TO_URLSAFE(b64) \
  char* p;                  \
  do {                      \
    p = strchr(b64, '+');   \
    if (p) *p = '-';        \
  } while (p);              \
                            \
  do {                      \
    p = strchr(b64, '/');   \
    if (p) *p = '_';        \
  } while (p);

static char* make_signature(unsigned char* mod, int mod_len,
                            unsigned char* exponent, int exponent_len, RSA* rsa,
                            unsigned char* cipher) {
  size_t sig_sz = 1 + 4 + RSA_size(rsa);
  unsigned char sig[sig_sz];
  sig[0] = 0;

  // 4 bytes padding + mod + 4 bytes padding + exponent
  size_t packed_key_sz = 4 + mod_len + 4 + exponent_len;
  unsigned char packed_key[packed_key_sz];
  packed_key[0] = 0;
  packed_key[1] = 0;
  packed_key[2] = 0;
  packed_key[3] = 128;
  memcpy(packed_key + 4, mod, mod_len);
  packed_key[4 + mod_len] = 0;
  packed_key[4 + mod_len + 1] = 0;
  packed_key[4 + mod_len + 2] = 0;
  packed_key[4 + mod_len + 3] = 3;
  memcpy(packed_key + 8 + mod_len, exponent, exponent_len);

  unsigned char* hash =
      SHA1((const unsigned char*)packed_key, packed_key_sz, NULL);

  memcpy(sig + 1, hash, 4);
  memcpy(sig + 1 + 4, cipher, RSA_size(rsa));

  char* ret = g_base64_encode(sig, sig_sz);
  B64_TO_URLSAFE(ret);
  return ret;
}

#undef B64_TO_URLSAFE

static unsigned char* make_cipher(char* email, char* passwd, RSA* rsa) {
  size_t email_len = strlen(email) + 1;
  size_t passwd_len = strlen(passwd);

  unsigned char tmp[email_len + passwd_len];
  memcpy(tmp, email, email_len);
  memcpy(tmp + email_len, passwd, passwd_len);

  unsigned char* cipher = malloc_or_die(RSA_size(rsa));
  RSA_public_encrypt(email_len + passwd_len, tmp, cipher, rsa,
                     RSA_PKCS1_OAEP_PADDING);

  return cipher;
}

typedef struct {
  char* data;
  size_t sz;
} WriteCBMem;

static WriteCBMem* writecbmem_new(void) {
  WriteCBMem* mem = malloc_or_die(sizeof(WriteCBMem));
  mem->data = malloc_or_die(1);
  mem->sz = 0;

  return mem;
}

static void writecbmem_free(WriteCBMem* mem) {
  free(mem->data);
  free(mem);
}

static size_t curl_write_cb(char* resp, size_t unused, size_t bufsz,
                            void* dst) {
  (void)unused;

  WriteCBMem* mem = (WriteCBMem*)dst;
  mem->data = realloc_or_die(mem->data, mem->sz + bufsz + 1);
  memcpy(&(mem->data[mem->sz]), resp, bufsz);
  mem->sz += bufsz;
  mem->data[mem->sz] = 0;

  return bufsz;
}

typedef struct {
  CURL* curl;
  ClientType type;
  struct curl_slist* headers;
  char* base_url;
} CURL_alloc;

static void CURL_alloc_free(CURL_alloc* curl_alloc) {
  curl_slist_free_all(curl_alloc->headers);
  curl_easy_cleanup(curl_alloc->curl);
  free(curl_alloc->base_url);
  free(curl_alloc);
}

static CURL_alloc* CURL_alloc_setup(ClientType t, const char* base_url,
                                    const char* token) {
  CURL_alloc* alloc = malloc_or_die(sizeof(CURL_alloc));

  CURL* curl;
  struct curl_slist* headers = NULL;

  curl = curl_easy_init();
  if (!curl) panic("curl_easy_init() failed");

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);

  if (CLIENT_AUTH == t) headers = curl_slist_append(headers, "Accept:");

  if (CLIENT_AUTH == t) {
    (void)token;
    headers = curl_slist_append(headers, "User-Agent: GoogleAuth/1.4");
    headers = curl_slist_append(
        headers, "Content-type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_URL,
                     "https://android.clients.google.com/auth");
    headers = curl_slist_append(headers, "Accept:");
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "identity");
  } else {
    headers = curl_slist_append(
        headers,
        "User-Agent: Notefinder/0.1 "
        "(https://github.com/i-desire-peace-where-i-live/nf)");
    headers = curl_slist_append(headers, "Content-type: application/json");
    headers = curl_slist_append(headers, "Connection: keep-alive");

    size_t auth_header_len =
        strlen("Authorization: OAuth ") + strlen(token) + 1;
    char auth_header[auth_header_len];

    strlcpy(auth_header, "Authorization: OAuth ", auth_header_len);
    strlcat(auth_header, token, auth_header_len);
    headers = curl_slist_append(headers, auth_header);

    curl_easy_setopt(curl, CURLOPT_URL, base_url);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate, br");
  }

  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(
      curl, CURLOPT_SSL_CIPHER_LIST,
      "ECDHE+AESGCM:ECDHE+CHACHA20:DHE+AESGCM:DHE+CHACHA20:ECDH+AESGCM:DH+"
      "AESGCM:ECDH+AES:DH+AES:RSA+AESGCM:RSA+AES:!aNULL:!eNULL:!MD5:!DSS");
  curl_easy_setopt(curl, CURLOPT_SSL_ENABLE_ALPN, 0L);

  alloc->curl = curl;
  alloc->headers = headers;
  alloc->type = t;

  if (base_url)
    alloc->base_url = strdup(base_url);
  else
    alloc->base_url = NULL;

  return alloc;
}

static long CURL_alloc_post(CURL_alloc* curl_alloc, char* body, WriteCBMem* dst,
                            const char* rel) {
  long response_code;
  CURLcode resp;

  curl_easy_setopt(curl_alloc->curl, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(curl_alloc->curl, CURLOPT_WRITEDATA, (void*)dst);

  if (rel && curl_alloc->base_url) {
    size_t full_url_len = strlen(curl_alloc->base_url) + strlen(rel) + 1;
    char full_url[full_url_len];
    strlcpy(full_url, curl_alloc->base_url, full_url_len);
    strlcat(full_url, rel, full_url_len);

    curl_easy_setopt(curl_alloc->curl, CURLOPT_URL, full_url);
  }

  resp = curl_easy_perform(curl_alloc->curl);

  if (CURLE_OK != resp) {
    debugf("curl_easy_perform() failed with: \"%s\"", curl_easy_strerror(resp));
    response_code = -1;
  } else {
    curl_easy_getinfo(curl_alloc->curl, CURLINFO_RESPONSE_CODE, &response_code);
  }

  return response_code;
}

static char* parse_response(char* resp, const char* key) {
  size_t tmpsz =
      strlen(key) + 2;  // two chars for preceeding newline and equation mark,
                        // e. g. '\nAuth=' or '\nToken='
  char tmp[tmpsz + 1];
  strlcpy(tmp, "\n", tmpsz + 1);
  strlcat(tmp, key, tmpsz + 1);
  strlcat(tmp, "=", tmpsz + 1);

  char* leftp = strstr(resp, tmp);
  char* rightp = strstr(leftp + tmpsz, "\n");

  if (!leftp) {
    debugf("couldn't parse auth token response");
    return NULL;
  }

  /* this is believed to be the last line in the file, no newline found */
  if (!rightp) rightp = resp + strlen(resp) + 1;

  if (!leftp || !rightp) {
    debugf("couldn't parse auth token response");
    return NULL;
  }

  int lindex = leftp - resp + tmpsz;
  int rindex = rightp - resp;

  size_t sz = rindex - lindex;

  char* ret = malloc_or_die(sz + 1);
  memcpy(ret, bytearr_sub(resp, lindex, rindex), sz);
  ret[sz + 1] = 0;

  return ret;
}

static char* get_master_token(CURL_alloc* curl_alloc, char* email,
                              char* passwd) {
  gsize key_sz;
  unsigned char* key = g_base64_decode(GOOGLE_API_KEY, &key_sz);

  int32_t mod_len = int32_from_bytearr(key, 0);
  int32_t exponent_len = int32_from_bytearr(key, 4 + mod_len);
  unsigned char* mod_bin = bytearr_sub(key, 4, 4 + mod_len);
  unsigned char* exponent_bin =
      bytearr_sub(key, 8 + mod_len, 8 + mod_len + exponent_len);
  free(key);
  BIGNUM* mod = BN_from_bytearr(mod_bin, mod_len);
  BIGNUM* exponent = BN_from_bytearr(exponent_bin, exponent_len);
  RSA* rsa = RSA_from_mod_exponent(mod, exponent);

  unsigned char* cipher = make_cipher(email, passwd, rsa);
  char* signature =
      make_signature(mod_bin, mod_len, exponent_bin, exponent_len, rsa, cipher);

  char* escaped_email = curl_easy_escape(curl_alloc->curl, email, 0);
  char* escaped_signature = curl_easy_escape(curl_alloc->curl, signature, 0);

  size_t signature_sz = strlen(MASTER_TOKEN_REQUEST) +
                        strlen(escaped_signature) + strlen(escaped_email);
  char* body = malloc_or_die(signature_sz + 1);

  debugf("generated signature: %s", signature);
  snprintf(body, signature_sz + 1, MASTER_TOKEN_REQUEST, escaped_email,
           escaped_signature);
  curl_free(escaped_email);
  curl_free(escaped_signature);
  passwd = NULL;

  debugf("master token POST request body: %s", body);
  WriteCBMem* resp = writecbmem_new();
  //  char* resp = NULL;
  long code = CURL_alloc_post(curl_alloc, body, resp, NULL);
  debugf("master token POST response code: %lld, body: %s", code, resp->data);

  if (200 != code) return NULL;

  char* token = parse_response(resp->data, "Token");
  writecbmem_free(resp);
out:
  free(mod_bin);
  free(exponent_bin);
  BN_free(mod);
  BN_free(exponent);
  free(cipher);
  free(signature);
  free(body);

  return token;
}

static char* get_final_token(CURL_alloc* curl_alloc, char* master_token) {
  char* escaped_master = curl_easy_escape(curl_alloc->curl, master_token, 0);

  size_t body_sz = strlen(FINAL_TOKEN_REQUEST) + strlen(escaped_master);
  char* body = malloc_or_die(body_sz + 1);

  snprintf(body, body_sz + 1, FINAL_TOKEN_REQUEST, escaped_master);
  curl_free(escaped_master);

  WriteCBMem* resp = writecbmem_new();
  long code = CURL_alloc_post(curl_alloc, body, resp, NULL);
  debugf("final token POST response code: %lld, body: %s", code, resp->data);

  if (200 != code) return NULL;

  char* ret = parse_response(resp->data, "Auth");
  writecbmem_free(resp);
  return ret;
}

/* FIXME! It's too ugly! */
static char* build_all_notes_request(void) {
  time_t unixtime = time(NULL);
  struct tm* local = localtime(&unixtime);

  char formatted[128];  // FIXME
  // strftime(formatted, 128, "%Y-%m-%dT%H:%M:%S.%fZ", local);
  strftime(formatted, 128, "%Y-%m-%dT%H:%M:%S.361720Z", local);

  int64_t session_id_1 = (int64_t)unixtime * 1000;
  int64_t session_id_2 = 9999999999;

  //  int32_t session_id_1 = ((int32_t)unixtime) * 1000;

  // return "s--%d--%d" % (int(tz * 1000), random.randint(1000000000,
  // 9999999999))

  size_t request_sz = strlen(ALL_NOTES_REQUEST) + 1000;  // FIXME
  char* request = malloc_or_die(request_sz);

  snprintf(request, request_sz, ALL_NOTES_REQUEST, formatted, session_id_1,
           session_id_2);

  return request;
}

int keep_backend_loop(ChildData child) {
  CURL_alloc* auth_curl;
  CURL_alloc* client_curl;

  auth_curl = CURL_alloc_setup(CLIENT_AUTH, NULL, NULL);
  (void)SSL_library_init();
  enable_debug = 0;

  char* master_token = get_master_token(auth_curl, NULL, NULL);

  if (!master_token) panicf("couldn't get master token");

  char* auth_token = get_final_token(auth_curl, master_token);

  if (!auth_token) panicf("couldn't get final token");

  free(master_token);

  char* all_notes_request = build_all_notes_request();

  client_curl = CURL_alloc_setup(
      CLIENT_KEEP, "https://www.googleapis.com/notes/v1/", auth_token);

  while (1) {
    WriteCBMem* entries_json = writecbmem_new();
    CURL_alloc_post(client_curl, all_notes_request, entries_json, "changes");

    Map* m = json_parse_object(&entries_json->data, NULL);
    Slice* entries_list = map_get(m, "nodes");

    const char* value;
    for (size_t i = 0; i < entries_list->len; i++) {
      value = map_get(entries_list->data[i], "title");

      if (value) send_client_data(child.fd0[1], 0, "title", value);

      value = map_get(entries_list->data[i], "text");

      if (value) send_client_data(child.fd0[1], 0, "text", value);
    }

    sleep(30);
  }

out:
  free(auth_token);
  CURL_alloc_free(auth_curl);
  CURL_alloc_free(client_curl);
openssl_clean:
  EVP_cleanup();
  CRYPTO_cleanup_all_ex_data();
}
