/*
 * The MIT License
 *
 * Copyright (c) 2010 - 2012 Shuhei Tanuma
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "php_git2.h"

PHPAPI zend_class_entry *git2_blob_class_entry;

static void php_git2_blob_free_storage(php_git2_blob *object TSRMLS_DC)
{
	if (object->blob != NULL) {
		git_blob_free(object->blob);
		object->blob = NULL;
	}
	zend_object_std_dtor(&object->zo TSRMLS_CC);
	efree(object);
}

zend_object_value php_git2_blob_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;

	PHP_GIT2_STD_CREATE_OBJECT(php_git2_blob);
	return retval;
}


/*
{{{ proto: Git2\Blob::__toString()
*/
PHP_METHOD(git2_blob, __toString)
{
	char *data;
	php_git2_blob *m_blob;
	
	m_blob = PHP_GIT2_GET_OBJECT(php_git2_blob, getThis());

	if (m_blob != NULL) {
		if (m_blob->blob == NULL) {
			RETURN_FALSE;
		}
		
		RETURN_STRINGL(git_blob_rawcontent(m_blob->blob), git_blob_rawsize(m_blob->blob),1);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


static zend_function_entry php_git2_blob_methods[] = {
	PHP_ME(git2_blob, __toString, NULL, ZEND_ACC_PUBLIC)
	{NULL,NULL,NULL}
};

void php_git2_blob_init(TSRMLS_D)
{
	zend_class_entry ce;
	
	INIT_NS_CLASS_ENTRY(ce, PHP_GIT2_NS, "Blob", php_git2_blob_methods);
	git2_blob_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	git2_blob_class_entry->create_object = php_git2_blob_new;
}