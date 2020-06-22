#!/bin/bash

file_path=$1

#upload file
upload_response=`https_proxy="" curl -k -H "Authorization: Bearer $BDBA_TOKEN" -H "Group: $BDBA_GROUP" -T $file_path "$BDBA_SERVER/api/upload/"`
product_id=`echo "$upload_response" | /NAS/tools/jq-linux64 -r '.results.product_id'`
if [ $product_id == "null" ]; then
    echo "Cannot upload file"
    exit 1
fi
report_url=`echo "$upload_response" | /NAS/tools/jq-linux64 -r '.results.report_url'`

echo "Scan upload completed - product id: $product_id ($report_url)"

set +e
MAX_RETRY=600

RETRY_COUNTER="0"
while [ $RETRY_COUNTER -lt $MAX_RETRY ]; do
	response=`https_proxy="" curl -s -X GET -H "Authorization: Bearer $BDBA_TOKEN" -k $BDBA_SERVER/api/product/$product_id/`
	CMD_RETURN_CODE=$?

	status=`echo "$response" | /NAS/tools/jq-linux64 -r '.results.status'`
	verdict=`echo "$response" | /NAS/tools/jq-linux64 -r '.results.summary.verdict.short'`
 
	if [ $CMD_RETURN_CODE == 0 ] && [[ $status == "R" ]]; then
	    echo $response | python -m json.tool
	    echo "Verdict: $verdict"
	    if [ $verdict == "Pass" ] || [ $verdict == "null" ]; then
		    exit 0
	    else
		    echo "There is a problem - please check report $report_url"
		    exit 1
	    fi 
	fi
	RETRY_COUNTER=$[$RETRY_COUNTER+1]
	echo "Scan not finished yet, [$RETRY_COUNTER/$MAX_RETRY] - checking again ... "
	sleep 20
done

set -e
if [ $RETRY_COUNTER -ge $MAX_RETRY ]; then
	exit 62
fi

exit 0
