function url_encode() {
echo "$@" \
| sed \
-e 's/%/%25/g' \
-e 's/ /%20/g' \
-e 's/!/%21/g' \
-e 's/"/%22/g' \
-e "s/'/%27/g" \
-e 's/#/%23/g' \
-e 's/(/%28/g' \
-e 's/)/%29/g' \
-e 's/+/%2b/g' \
-e 's/,/%2c/g' \
-e 's/-/%2d/g' \
-e 's/:/%3a/g' \
-e 's/;/%3b/g' \
-e 's/?/%3f/g' \
-e 's/@/%40/g' \
-e 's/\$/%24/g' \
-e 's/\&/%26/g' \
-e 's/\*/%2a/g' \
-e 's/\./%2e/g' \
-e 's/\//%2f/g' \
-e 's/\[/%5b/g' \
-e 's/\\/%5c/g' \
-e 's/\]/%5d/g' \
-e 's/\^/%5e/g' \
-e 's/_/%5f/g' \
-e 's/`/%60/g' \
-e 's/{/%7b/g' \
-e 's/|/%7c/g' \
-e 's/}/%7d/g' \
-e 's/~/%7e/g'
}

PRODUCT_NAME="BT D/D"
PROJECT_NAME="rose"
en_product=`url_encode $PRODUCT_NAME`
en_project=`url_encode $PROJECT_NAME`
report_json=`cat project_info.json`

curl --noproxy "*" -v -i -k -H "Content-Type: application/json; charset=utf-8" -d "$report_json" "https://spp.sec.samsung.net/api/data/tem/update?product=${en_product}&project=${en_project}"
