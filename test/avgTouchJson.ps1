$velocitat = 9600
#$velocitat = 115200
$RtsEnable = $true;
$DtrEnable = $true;
$hex = $false
$volca = $true
$volcarNomArxiu= "~\volcatSerie.bin"

function triaComPort(){
    $selection = [System.IO.Ports.SerialPort]::getportnames() | select -Unique

    If($selection.Count -gt 0){
        $title = "Serial port selection, speed: $velocitat"
        $message = "Which port would you like to use?"

        # Build the choices menu
        $options = @()
        
        For($index = 0; $index -lt $selection.Count; $index++){    
            $options += New-Object -TypeName  System.Management.Automation.Host.ChoiceDescription -ArgumentList @($selection[$index],($index+1))
        
        }

        
        $result = $host.ui.PromptForChoice($title, $message, $options, 0) 

        $selection = $selection[$result]
    }

    return $selection
}
function tancaComPort(){
    $port.Close()
    $port.Dispose()
    Set-Content $volcarNomArxiu $al.ToArray() -enc byte
}
if($volca){
    try{rm $volcarNomArxiu}catch{}
    $al = new-object collections.generic.list[byte]
}
$port = triaComPort

if(!$port){"Must choose";return}

Write-Host $("Port:"+$port)

#$port= new-Object System.IO.Ports.SerialPort $port,$velocitat,$([System.IO.Ports.Parity]::None) , 8, $([System.IO.Ports.StopBits]::None)
$port= new-Object System.IO.Ports.SerialPort $port,$velocitat,None , 8, $([System.IO.Ports.StopBits]::One)

$port.ReadTimeout = 500;
$port.WriteTimeout = 500;
$port.RtsEnable = $RtsEnable;
$port.DtrEnable = $DtrEnable;
$linies = @()
[bool]$dirTancat = $false
try{
    while($true){
        
        if(!$port.IsOpen){
            try{
                $port.Open()
                write-host "+" 
            }catch{
                if(!$dirTancat){
                    $dirTancat = $true
                    write-host "-" -NoNewline
                }
            }
        }
        if($port.BytesToRead -gt 0){
            $linies += $port.ReadLine()
            Write-Host "."
            
        }
        if ([Console]::KeyAvailable)
        {
            $hola = [Console]::ReadKey($true).keyChar
            if($hola -eq 13){
                $port.Write([char](10))
            }elseif($hola -eq 2700){
                tancaComPort
                Write-Host "--Port $($port.PortName) tancat, pots flashejar--"
                Pause
            }else{
                $port.Write($hola)

            }
            
            
            
        }

    }
    tancaComPort
}catch{

    if($Error[0].InvocationInfo.Line.Contains("KeyAvailable")){
        Write-Host "ho pots anar per el ISE, no és interactiu"
    }
    tancaComPort
}finally{
    tancaComPort
}
write-host "adeu"
$($linies |%{$_ | ConvertFrom-Json}) | measure -Minimum -Maximum -Average -Property x,y,p
