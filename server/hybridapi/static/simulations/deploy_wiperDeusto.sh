#!/bin/bash
#
# Deploys the built babylon app to the S3 bucket.
#

# Note: As of now some files are .gz and need to be served through gzip encoding by the 
# web server or s3. In s3 this requires setting up metadata to set the content-encoding to gzip.

# Upload the ./wiperDeusto folder to the simulations/wiperDeusto/ directory.
aws s3 sync --region eu-central-1 wiperDeusto s3://ll-static-apps/simulations/wiperDeusto


aws s3 cp --region eu-central-1 wiperDeusto/Build/CarWiperDeusto.data.gz s3://ll-static-apps/simulations/wiperDeusto/Build/CarWiperDeusto.data.gz --content-encoding gzip --metadata-directive REPLACE
aws s3 cp --region eu-central-1 wiperDeusto/Build/CarWiperDeusto.framework.js.gz s3://ll-static-apps/simulations/wiperDeusto/Build/CarWiperDeusto.framework.js.gz --content-encoding gzip --metadata-directive REPLACE
aws s3 cp --region eu-central-1 wiperDeusto/Build/CarWiperDeusto.wasm.gz s3://ll-static-apps/simulations/wiperDeusto/Build/CarWiperDeusto.wasm.gz --content-encoding gzip --metadata-directive REPLACE


# Invalidate cloudfront cache.
aws cloudfront create-invalidation --distribution-id E33MK6W8RMC191 --paths "/simulations/wiperDeusto/*"
