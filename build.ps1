# build.ps1
# fire up iot:magic with . mapped as a volume and do a pio build

$commandString = "docker run " +
"-it --user 1000:1000 --group-add=dialout --group-add=ubuntu " +
"--volume=$(Get-Location):/home/ubuntu/project hamishcunningham/iot:magic " +
"pio run $args"

Write-Host "running docker build: $commandString"
Invoke-Expression $commandString
