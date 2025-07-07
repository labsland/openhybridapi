#!/bin/bash
#
# Deploys the built babylon app to the S3 bucket.
#

# Note: As of now some files are .gz and need to be served through gzip encoding by the 
# web server or s3. In s3 this requires setting up metadata to set the content-encoding to gzip.

# Upload the ./wiper2BitDeusto folder to the simulations/wiper2BitDeusto/ directory.
aws s3 sync --region eu-central-1 wiper2BitDeusto s3://ll-static-apps/simulations/wiper2BitDeusto


aws s3 cp --region eu-central-1 wiper2BitDeusto/Build/CarWiperDeusto2Bit.data.gz s3://ll-static-apps/simulations/wiper2BitDeusto/Build/CarWiperDeusto2Bit.data.gz --content-encoding gzip --metadata-directive REPLACE
aws s3 cp --region eu-central-1 wiper2BitDeusto/Build/CarWiperDeusto2Bit.framework.js.gz s3://ll-static-apps/simulations/wiper2BitDeusto/Build/CarWiperDeusto2Bit.framework.js.gz --content-encoding gzip --metadata-directive REPLACE
aws s3 cp --region eu-central-1 wiper2BitDeusto/Build/CarWiperDeusto2Bit.wasm.gz s3://ll-static-apps/simulations/wiper2BitDeusto/Build/CarWiperDeusto2Bit.wasm.gz --content-encoding gzip --metadata-directive REPLACE


# Invalidate cloudfront cache.
aws cloudfront create-invalidation --distribution-id E33MK6W8RMC191 --paths "/simulations/wiper2BitDeusto/*"
