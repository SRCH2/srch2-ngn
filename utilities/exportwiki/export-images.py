import os
import os.path

path = './wiki/'

for fname in os.listdir(path):
    if not fname.endswith('.html'):
        continue
    fname_full = os.path.join(path, fname) 
    txt = open(fname_full).read()
    img = txt.rfind('<img')
    while (img > 0):
        idx_st = txt.find('src="', img) + 5
        idx_en = txt.find('"', idx_st)

        img_url = txt[idx_st:idx_en]
        #if img_url.endswith('?format=raw'):
        img_url = img_url[len('/wiki/bimaple'):]
        img_file = img_url[img_url.rfind('/') + 1:]
        print img_url, img_file
        txt = txt[:idx_st] + "media/" + img_file + txt[idx_en:]
        img = txt.rfind('<img', 0, img)
    open(fname_full, 'w').write(txt)
