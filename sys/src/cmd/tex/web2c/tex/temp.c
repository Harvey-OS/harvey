void printcmdchr ( quarterword cmd , halfword chrcode ) 
{printcmdchr_regmem 
  switch ( cmd ) 
  {case 1 : 
    {
      print ( 553 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 2 : 
    {
      print ( 554 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 3 : 
    {
      print ( 555 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 6 : 
    {
      print ( 556 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 7 : 
    {
      print ( 557 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 8 : 
    {
      print ( 558 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 9 : 
    print ( 559 ) ; 
    break ; 
  case 10 : 
    {
      print ( 560 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 11 : 
    {
      print ( 561 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 12 : 
    {
      print ( 562 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 75 : 
  case 76 : 
    if ( chrcode < 10300 ) 
    printskipparam ( chrcode - 10282 ) ; 
    else if ( chrcode < 10556 ) 
    {
      printesc ( 391 ) ; 
      printint ( chrcode - 10300 ) ; 
    } 
    else {
	
      printesc ( 392 ) ; 
      printint ( chrcode - 10556 ) ; 
    } 
    break ; 
  case 72 : 
    if ( chrcode >= 10822 ) 
    {
      printesc ( 403 ) ; 
      printint ( chrcode - 10822 ) ; 
    } 
    else switch ( chrcode ) 
    {case 10813 : 
      printesc ( 394 ) ; 
      break ; 
    case 10814 : 
      printesc ( 395 ) ; 
      break ; 
    case 10815 : 
      printesc ( 396 ) ; 
      break ; 
    case 10816 : 
      printesc ( 397 ) ; 
      break ; 
    case 10817 : 
      printesc ( 398 ) ; 
      break ; 
    case 10818 : 
      printesc ( 399 ) ; 
      break ; 
    case 10819 : 
      printesc ( 400 ) ; 
      break ; 
    case 10820 : 
      printesc ( 401 ) ; 
      break ; 
      default: 
      printesc ( 402 ) ; 
      break ; 
    } 
    break ; 
  case 73 : 
    if ( chrcode < 12718 ) 
    printparam ( chrcode - 12663 ) ; 
    else {
	
      printesc ( 472 ) ; 
      printint ( chrcode - 12718 ) ; 
    } 
    break ; 
  case 74 : 
    if ( chrcode < 13251 ) 
    printlengthparam ( chrcode - 13230 ) ; 
    else {
	
      printesc ( 496 ) ; 
      printint ( chrcode - 13251 ) ; 
    } 
    break ; 
  case 45 : 
    printesc ( 504 ) ; 
    break ; 
  case 90 : 
    printesc ( 505 ) ; 
    break ; 
  case 40 : 
    printesc ( 506 ) ; 
    break ; 
  case 41 : 
    printesc ( 507 ) ; 
    break ; 
  case 77 : 
    printesc ( 515 ) ; 
    break ; 
  case 61 : 
    printesc ( 508 ) ; 
    break ; 
  case 42 : 
    printesc ( 527 ) ; 
    break ; 
  case 16 : 
    printesc ( 509 ) ; 
    break ; 
  case 107 : 
    printesc ( 500 ) ; 
    break ; 
  case 88 : 
    printesc ( 514 ) ; 
    break ; 
  case 15 : 
    printesc ( 510 ) ; 
    break ; 
  case 92 : 
    printesc ( 511 ) ; 
    break ; 
  case 67 : 
    printesc ( 501 ) ; 
    break ; 
  case 62 : 
    printesc ( 512 ) ; 
    break ; 
  case 64 : 
    printesc ( 32 ) ; 
    break ; 
  case 102 : 
    printesc ( 513 ) ; 
    break ; 
  case 32 : 
    printesc ( 516 ) ; 
    break ; 
  case 36 : 
    printesc ( 517 ) ; 
    break ; 
  case 39 : 
    printesc ( 518 ) ; 
    break ; 
  case 37 : 
    printesc ( 327 ) ; 
    break ; 
  case 44 : 
    printesc ( 47 ) ; 
    break ; 
  case 18 : 
    printesc ( 348 ) ; 
    break ; 
  case 46 : 
    printesc ( 519 ) ; 
    break ; 
  case 17 : 
    printesc ( 520 ) ; 
    break ; 
  case 54 : 
    printesc ( 521 ) ; 
    break ; 
  case 91 : 
    printesc ( 522 ) ; 
    break ; 
  case 34 : 
    printesc ( 523 ) ; 
    break ; 
  case 65 : 
    printesc ( 524 ) ; 
    break ; 
  case 103 : 
    printesc ( 525 ) ; 
    break ; 
  case 55 : 
    printesc ( 332 ) ; 
    break ; 
  case 63 : 
    printesc ( 526 ) ; 
    break ; 
  case 66 : 
    printesc ( 529 ) ; 
    break ; 
  case 96 : 
    printesc ( 530 ) ; 
    break ; 
  case 0 : 
    printesc ( 531 ) ; 
    break ; 
  case 98 : 
    printesc ( 532 ) ; 
    break ; 
  case 80 : 
    printesc ( 528 ) ; 
    break ; 