<#
Comanda al Open On-Chip Debugger OCD: x/400bx myCharBuffer
info break
delete
delete 2
#>
$linies = [System.IO.File]::ReadAllLines("scripts\ocd.dump")
$resultat = @()
$index = 0

foreach($linia in $linies){
    # Extract hex values from the line after the colon
    $hexValues = $linia.split(":")[1].split("`t") | Where-Object {$_}

    foreach($hexValue in $hexValues){
        # Remove '0x' prefix and convert to decimal
        $decimalValue = [Convert]::ToInt32($hexValue, 16)

        # Convert to character (printable or show as [dec])
        $char = if ($decimalValue -ge 32 -and $decimalValue -le 126) {
            [char]$decimalValue
        } else {
            "[$decimalValue]"
        }

        # Create hashtable for this byte
        $resultat += [PSCustomObject]@{
            Index = $index
            Hex = $hexValue
            Decimal = $decimalValue
            Character = $char
        }

        $index++
    }
}

$resultat