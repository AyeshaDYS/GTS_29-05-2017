<?php
$page_security = 'SA_SUPPLIERCREDIT';
$path_to_root = "..";

include_once($path_to_root . "/purchasing/includes/supp_debit_class.inc");

include_once($path_to_root . "/includes/session.inc");

include_once($path_to_root . "/includes/data_checks.inc");

include_once($path_to_root . "/purchasing/includes/purchasing_db.inc");


include_once($path_to_root . "/purchasing/includes/purchasing_ui.inc");

$js = "";
if ($use_popup_windows)
	$js .= get_js_open_window(900, 500);
if ($use_date_picker)
	$js .= get_js_date_picker();
page(_($help_context = "Supplier Debit Note"), false, false, "", $js);

//----------------------------------------------------------------------------------------

check_db_has_suppliers(_("There are no suppliers defined in the system."));

//---------------------------------------------------------------------------------------------------------------

if (isset($_GET['AddedID'])) 
{
	$invoice_no = $_GET['AddedID'];
	$trans_type = ST_DEBIT;


    echo "<center>";
    display_notification_centered(_("Supplier debit note has been processed."));
    display_note(get_supplier_trans_view_str_consignment($trans_type, $invoice_no, _("View this Credit Note")));

	display_note(get_gl_view_str($trans_type, $invoice_no, _("View the GL Journal Entries for this Credit Note")), 1);

    hyperlink_params($_SERVER['PHP_SELF'], _("Enter Another Credit Note"), "New=1");
	hyperlink_params("$path_to_root/admin/attachments.php", _("Add an Attachment"), "filterType=$trans_type&trans_no=$invoice_no");
	
	display_footer_exit();
}

//---------------------------------------------------------------------------------------------------

if (isset($_GET['New']))
{
	if (isset( $_SESSION['DEBIT']))
	{
		unset ($_SESSION['DEBIT']->grn_items);
		unset ($_SESSION['DEBIT']->gl_codes);
		unset ($_SESSION['DEBIT']);
	}

	$_SESSION['DEBIT'] = new supp_trans_debit(ST_DEBIT);


//	if (isset($_GET['invoice_no']))
//	{
//		$_SESSION['DEBIT']->supp_reference = $_POST['invoice_no'] = $_GET['invoice_no'];
//	}
}

function clear_fields()
{
	global $Ajax;
	
	unset($_POST['gl_code']);
	unset($_POST['dimension_id']);
	unset($_POST['dimension2_id']);
	unset($_POST['amount']);
	unset($_POST['memo_']);
	unset($_POST['AddGLCodeToTrans']);
	$Ajax->activate('gl_items');
	set_focus('gl_code');
}
//------------------------------------------------------------------------------------------------
//	GL postings are often entered in the same form to two accounts
//  so fileds are cleared only on user demand.
//

if (isset($_POST['ClearFields']))
{
	clear_fields();
}

if (isset($_POST['AddGLCodeToTrans'])){

	$Ajax->activate('gl_items');
	$input_error = false;

	$result = get_gl_account_info($_POST['gl_code']);
	if (db_num_rows($result) == 0)
	{
		display_error(_("The account code entered is not a valid code, this line cannot be added to the transaction."));
		set_focus('gl_code');
		$input_error = true;
	}
	else
	{
		$myrow = db_fetch_row($result);
		$gl_act_name = $myrow[1];
		if (!check_num('amount'))
		{
			display_error(_("The amount entered is not numeric. This line cannot be added to the transaction."));
			set_focus('amount');
			$input_error = true;
		}
	}

	if (!is_tax_gl_unique(get_post('gl_code'))) {
   		display_error(_("Cannot post to GL account used by more than one tax type."));
		set_focus('gl_code');
   		$input_error = true;
	}

	if ($input_error == false)
	{
		$_SESSION['DEBIT']->add_gl_codes_to_trans($_POST['gl_code'], $gl_act_name,
			$_POST['dimension_id'], $_POST['dimension2_id'], 
			input_num('amount'), $_POST['memo_']);
		set_focus('gl_code');
	}
}


//---------------------------------------------------------------------------------------------------

function check_data()
{
	global $total_grn_value, $total_gl_value, $Refs, $SysPrefs;
	
	if (!$_SESSION['DEBIT']->is_valid_trans_to_post())
	{
		display_error(_("The credit note cannot be processed because the there are no items or values on the invoice.  Credit notes are expected to have a charge."));
		set_focus('');
		return false;
	}

	if (!$Refs->is_valid($_SESSION['DEBIT']->reference)) 
	{
		display_error(_("You must enter an credit note reference."));
		set_focus('reference');
		return false;
	}

	if (!is_new_reference($_SESSION['DEBIT']->reference, ST_DEBIT))
	{
		display_error(_("The entered reference is already in use."));
		set_focus('reference');
		return false;
	}
//
//	if (!$Refs->is_valid($_SESSION['DEBIT']->supp_reference))
//	{
//		display_error(_("You must enter a supplier's credit note reference."));
//		set_focus('supp_reference');
//		return false;
//	}

	if (!is_date($_SESSION['DEBIT']->tran_date))
	{
		display_error(_("The credit note as entered cannot be processed because the date entered is not valid."));
		set_focus('tran_date');
		return false;
	} 
	elseif (!is_date_in_fiscalyear($_SESSION['DEBIT']->tran_date)) 
	{
		display_error(_("The entered date is not in fiscal year."));
		set_focus('tran_date');
		return false;
	}
//	if (!is_date( $_SESSION['DEBIT']->due_date))
//	{
//		display_error(_("The invoice as entered cannot be processed because the due date is in an incorrect format."));
//		set_focus('due_date');
//		return false;
//	}

	if ($_SESSION['DEBIT']->ov_amount < ($total_gl_value + $total_grn_value))
	{
		display_error(_("The credit note total as entered is less than the sum of the the general ledger entires (if any) and the charges for goods received. There must be a mistake somewhere, the credit note as entered will not be processed."));
		return false;
	}

	if (!$SysPrefs->allow_negative_stock()) {
		foreach ($_SESSION['DEBIT']->grn_items as $n => $item) {
			if (is_inventory_item($item->item_code))
			{
				$qoh = get_qoh_on_date($item->item_code, null, $_SESSION['DEBIT']->tran_date);
				if ($item->this_quantity_inv > $qoh)
				{
					$stock = get_item($item->item_code);
					display_error(_("The return cannot be processed because there is an insufficient quantity for item:") .
						" " . $stock['stock_id'] . " - " . $stock['description'] . " - " .
						_("Quantity On Hand") . " = " . number_format2($qoh, get_qty_dec($stock['stock_id'])));
					return false;
				}
				return true;
			}
		}
	}
	return true;
}

//---------------------------------------------------------------------------------------------------

function handle_commit_debit_note()
{
	copy_to_trans_debit($_SESSION['DEBIT']);



	if (!check_data())
		return;

	if (isset($_POST['invoice_no']))
		$invoice_no = add_supp_invoice_debit($_SESSION['DEBIT'], $_POST['invoice_no']);
	else
		$invoice_no = add_supp_invoice_debit($_SESSION['DEBIT']);

    $_SESSION['DEBIT']->clear_items();
    unset($_SESSION['DEBIT']);

	meta_forward($_SERVER['PHP_SELF'], "AddedID=$invoice_no");
}

//--------------------------------------------------------------------------------------------------

if (isset($_POST['PostCreditNote']))
{
	handle_commit_debit_note();
}

function check_item_data($n)
{

//	if (!check_num('This_QuantityCredited'.$n, 0))
//	{
//		display_error(_("The quantity to credit must be numeric and greater than zero."));
//		set_focus('This_QuantityCredited'.$n);
//		return false;
//	}
//
	if (!check_num('ChgPrice'.$n, 0))
	{
		display_error(_("The price is either not numeric or negative."));
		set_focus('ChgPrice'.$n);
		return false;
	}

	return true;
}

function commit_item_data($n)
{
	if (check_item_data($n))
	{

		$_SESSION['DEBIT']->add_grn_to_trans_debit($n, $_POST['item_code'.$n],
    		$_POST['description'.$n], 0,
    		0,0,$_POST['order_price'.$n], input_num('ChgPrice'.$n),
    		$_POST['std_cost_unit'.$n],$_POST['units'.$n]
    		,$_POST['category'.$n],$_POST['part_no'.$n],$_POST['item_batch_no'.$n],
			$_POST['serial_no'.$n],$_POST['long_description'.$n]);
		
	}

}

//-----------------------------------------------------------------------------------------

$id = find_submit('grn_item_id');
if ($id != -1)
{
	commit_item_data($id);
}
//
//if (isset($_POST['InvGRNAll']))
//{
//   	foreach($_POST as $postkey=>$postval )
//    {
//		if (strpos($postkey, "qty_recd") === 0)
//		{
//			$id = substr($postkey, strlen("qty_recd"));
//			$id = (int)$id;
//			commit_item_data($id);
//		}
//    }
//}


//--------------------------------------------------------------------------------------------------
$id3 = find_submit('Delete');
if ($id3 != -1)
{
	$_SESSION['DEBIT']->remove_grn_from_trans($id3);
	$Ajax->activate('grn_items');
	$Ajax->activate('inv_tot');
}

$id4 = find_submit('Delete2');
if ($id4 != -1)
{
	$_SESSION['DEBIT']->remove_gl_codes_from_trans($id4);
	clear_fields();
	$Ajax->activate('gl_items');
	$Ajax->activate('inv_tot');
}
if (isset($_POST['RefreshInquiry']))
{
	$Ajax->activate('grn_items');
	$Ajax->activate('inv_tot');
}

if (isset($_POST['go']))
{
	$Ajax->activate('gl_items');
	display_quick_entries($_SESSION['DEBIT'], $_POST['qid'], input_num('totamount'),
		QE_SUPPINV);
	$_POST['totamount'] = price_format(0); $Ajax->activate('totamount');
	$Ajax->activate('inv_tot');
}

//--------------------------------------------------------------------------------------------------

start_form();

invoice_header_supp_debit($_SESSION['DEBIT']);

if ($_POST['supplier_id']=='') 
	display_error('No supplier found for entered search text');
else {
	$total_grn_value = display_grn_items_debit($_SESSION['DEBIT'], 1);


	$total_gl_value = display_gl_items_debit($_SESSION['DEBIT'], 1);

	div_start('inv_tot');
	invoice_totals_debit($_SESSION['DEBIT']);
	div_end();
}

if ($id != -1)
{
	$Ajax->activate('grn_items');
	$Ajax->activate('inv_tot');
}

if (get_post('AddGLCodeToTrans'))
	$Ajax->activate('inv_tot');

br();
submit_center('PostCreditNote', _("Enter Credit Note"), true, '', 'default');
br();

end_form();
end_page();
?>
