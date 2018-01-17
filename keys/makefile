all: testkey0 testkey1 testkey2 testkey3 testkey4 testkey5 testkey6 testkey7 testkey8

keygen: keygen.c sha256.c badgekey.c
	gcc keygen.c sha256.c badgekey.c -o keygen

keydata.c: keygen
	./keygen > keydata.c

testkeydeps=testkey.c sha256.c badgekey.c keydata.c

testkey0: $(testkeydeps)
	gcc $(testkeydeps) -o $@ -DBADGETYPE0

testkey1: $(testkeydeps)
	gcc $(testkeydeps) -o $@ -DBADGETYPE1

testkey2: $(testkeydeps)
	gcc $(testkeydeps) -o $@ -DBADGETYPE2

testkey3: $(testkeydeps)
	gcc $(testkeydeps) -o $@ -DBADGETYPE3

testkey4: $(testkeydeps)
	gcc $(testkeydeps) -o $@ -DBADGETYPE4

testkey5: $(testkeydeps)
	gcc $(testkeydeps) -o $@ -DBADGETYPE5

testkey6: $(testkeydeps)
	gcc $(testkeydeps) -o $@ -DBADGETYPE6

testkey7: $(testkeydeps)
	gcc $(testkeydeps) -o $@ -DBADGETYPE7

testkey8: $(testkeydeps)
	gcc $(testkeydeps) -o $@ -DBADGETYPE8